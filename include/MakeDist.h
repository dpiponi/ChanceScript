#pragma once

template<typename ProbType> struct TDistSample
{
    TDistSample(int InSize) : Size(InSize), R(0) {}

    int Current() const { return R; }

    bool Next() { return ++R >= Size; }

    int Size;
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
            if (EnumeratorStack.back().Next())
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
        int R;
        if (CurrentEnumerator < EnumeratorStack.size())
        {
            R = EnumeratorStack[CurrentEnumerator].Current();
        }
        else
        {
            auto RollEnumerator = TDistSample<ProbType>(Dist.PDF.size());
            R = RollEnumerator.Current();
            EnumeratorStack.push_back(std::move(RollEnumerator));
        }
        T Result = Dist.PDF[R].Value;
        Importance *= Dist.PDF[R].Prob;
        ++CurrentEnumerator;
        return Result;
    }

    std::vector<TDistSample<ProbType>> EnumeratorStack;
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
