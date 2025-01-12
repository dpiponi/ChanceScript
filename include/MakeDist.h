#pragma once

struct FEnumeratorBase
{
    virtual ~FEnumeratorBase() {}

    virtual bool Next() = 0;
    virtual void Current(void* Result, void* Prob) const = 0;
};

template<typename ProbType, typename T>
struct TDistSample : public FEnumeratorBase
{
    TDistSample(const TDist<ProbType, T>& InDist) : Dist(InDist), R(0) {}

    void Current(void* Address, void* Prob) const override
    {
        *(T*)Address = Dist.PDF[R].Value;
        *(ProbType*)Prob = Dist.PDF[R].Prob;
    }

    bool Next() override
    {
        ++R;
        return R >= Dist.PDF.size();
    }

    TDist<ProbType, T> Dist;
    int R;
};

struct FSampler
{
    template<typename ProbType, typename T>
    T operator()(const TDist<ProbType, T>& Dist);
};

template<typename ProbType> struct TExplorerBase : public FSampler
{
    TExplorerBase() : CurrentEnumerator(0), Importance(1.) {}

    void Restart()
    {
        CurrentEnumerator = 0;
        Importance = 1;
    }

    bool IncrementState()
    {
        while (!EnumeratorStack.empty() > 0)
        {
            if (EnumeratorStack.back()->Next())
            {
                EnumeratorStack.pop_back();
            }
            else
            {
                return false;
            }
        }
        return true;
    }

    template<typename T> T operator()(const TDist<ProbType, T>& Dist)
    {
        T Result;
        ProbType Prob;
        if (CurrentEnumerator < EnumeratorStack.size())
        {
            EnumeratorStack[CurrentEnumerator]->Current(&Result, &Prob);
        }
        else
        {
            // XXX no need for this as `Dist` is passed in.
            auto RollEnumerator =
                std::make_unique<TDistSample<ProbType, T>>(Dist);
            RollEnumerator->Current(&Result, &Prob);
            EnumeratorStack.push_back(std::move(RollEnumerator));
        }
        Importance *= Prob;
        ++CurrentEnumerator;
        return Result;
    }

    std::vector<std::unique_ptr<FEnumeratorBase>> EnumeratorStack;
    int CurrentEnumerator;
    ProbType Importance;
};

template<typename ProbType, typename T>
T FSampler::operator()(const TDist<ProbType, T>& Dist)
{
    return reinterpret_cast<TExplorerBase<ProbType>*>(this)->operator()(Dist);
}

template<typename Prob, typename T>
struct TExplorer : public TExplorerBase<Prob>
{
    void Record(const T& Result)
    {
        PDF.emplace_back(Result, TExplorerBase<Prob>::Importance);
    }

    TDist<Prob, T> GetDist() const { return TDist<Prob, T>(PDF); }

    std::vector<TAtom<Prob, T>> PDF;
};

template<typename ProbType, typename FType, typename... ArgTypes>
TDist<ProbType,
      std::invoke_result_t<FType, TExplorerBase<ProbType>&, ArgTypes...>>
MakeDist(FType F, ArgTypes... Args)
{
    TExplorer<
        ProbType,
        std::invoke_result_t<FType, TExplorerBase<ProbType>&, ArgTypes...>>
        Explorer;

    do
    {
        Explorer.Restart();
        Explorer.Record(F(Explorer, Args...));
    } while (!Explorer.IncrementState());

    return Explorer.GetDist();
}

template<typename FType, typename... ArgTypes>
TDist<double, std::invoke_result_t<FType, TExplorerBase<double>&, ArgTypes...>>
MakeDDist(FType F, ArgTypes... Args)
{
    return MakeDist<double>(F, Args...);
}
