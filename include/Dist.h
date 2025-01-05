#pragma once

template<typename ProbType, typename ValueType> struct FAtom
{
    ValueType Value;
    ProbType  Prob;

    auto operator<=>(const FAtom& Other) const { return Value <=> Other.Value; }
};

template<typename ProbType, typename ValueType> struct TDist;

// For convenience `TDist<>` objects support overloading of arithmetic operators
// and these are helpers to determine the correct return type.
template<typename ValueType1, typename ValueType2>
using AddResultType =
    decltype(std::declval<ValueType1>() + std::declval<ValueType2>());

template<typename ValueType1, typename ValueType2>
using TimesResultType =
    decltype(std::declval<ValueType1>() * std::declval<ValueType2>());

template<typename ProbType, typename ValueType> struct TDist
{
    TDist(std::initializer_list<FAtom<ProbType, ValueType>> InPDF) : PDF(InPDF)
    {
    }

    // Operators
    template<typename OtherType>
    TDist<ProbType, AddResultType<ValueType, OtherType>>
    operator+(const TDist<ProbType, OtherType>& Other) const
    {
        return AndThen(
            [&Other](int x)
            { return Other.Transform([x](int y) { return x + y; }); });
    }

    template<typename OtherType>
    TDist<ProbType, bool>
    operator<=(const TDist<ProbType, OtherType>& Other) const
    {
        return AndThen(
            [&Other](int x)
            { return Other.Transform([x](int y) { return x <= y; }); });
    }

    template<typename OtherType>
    TDist<ProbType, bool>
    operator>=(const TDist<ProbType, OtherType>& Other) const
    {
        return AndThen(
            [&Other](int x)
            { return Other.Transform([x](int y) { return x >= y; }); });
    }

    template<typename OtherType>
    TDist<ProbType, AddResultType<ValueType, OtherType>>
    operator+(const OtherType& Other) const
    {
        return Transform([&Other](int x) { return x + Other; });
    }

    template<typename OtherType>
    TDist<ProbType, bool> operator<=(const OtherType& Other) const
    {
        return Transform([&Other](int x) { return x <= Other; });
    }

    template<typename OtherType>
    TDist<ProbType, bool> operator>=(const OtherType& Other) const
    {
        return Transform([&Other](int x) { return x >= Other; });
    }

    // Assumes values are sorted.  Merges two successive values together if they
    // are in fact equal.
    void Merge()
    {
        std::vector<FAtom<ProbType, ValueType>> Merged;
        for (const auto& FAtom : PDF)
        {
            if (!Merged.empty() && Merged.back().Value == FAtom.Value)
            {
                Merged.back().Prob += FAtom.Prob;
            }
            else
            {
                Merged.push_back(FAtom);
            }
        }

        PDF = std::move(Merged);
    }

    void Sort() { std::sort(PDF.begin(), PDF.end()); }

    void remove_zero()
    {
        PDF.erase(std::remove_if(PDF.begin(),
                                 PDF.end(),
                                 [](const FAtom<ProbType, ValueType>& p)
                                 { return p.Prob == 0; }),
                  PDF.end());
    }

    void chop(ProbType eps)
    {
        PDF.erase(std::remove_if(PDF.begin(),
                                 PDF.end(),
                                 [eps](const FAtom<ProbType, ValueType>& p)
                                 { return abs(p.Prob) < eps; }),
                  PDF.end());
    }

    void canonicalise()
    {
        Sort();
        Merge();
        remove_zero();
    }

    template<typename F>
    std::invoke_result_t<F, ValueType> AndThen(const F& f) const
    {
        std::invoke_result_t<F, ValueType> result{};
        for (const auto& x : PDF)
        {
            auto   fx = f(x.Value);
            double Prob = x.Prob;
            for (auto& r : fx.PDF)
            {
                result.PDF.push_back(FAtom{ r.Value, Prob * r.Prob });
            }
        }

        result.canonicalise();

        return result;
    }

    template<typename F>
    TDist<ProbType, std::invoke_result_t<F, ValueType>>
    Transform(const F& f) const
    {
        TDist<ProbType, std::invoke_result_t<F, ValueType>> result{};
        for (const auto& x : PDF)
        {
            auto fx = f(x.Value);
            result.PDF.push_back(FAtom{ fx, x.Prob });
        }

        result.canonicalise();

        return result;
    }

    template<typename F>
    std::invoke_result_t<F, ValueType> operator>>(const F& f) const
    {
        return this->AndThen(f);
    }

    void check() const
    {
        double total = 0.0;

        for (const auto& [Value, Prob] : PDF)
        {
            total += Prob;
        }

        std::cout << "total = " << total << std::endl;
    }

    void dump() const
    {
        for (const auto& [Value, Prob] : PDF)
        {
            std::cout << Value << ": " << Prob << std::endl;
        }
    }

    const std::vector<FAtom<ProbType, ValueType>>& GetPDF() const
    {
        return PDF;
    }

    auto begin() const
    {
        return PDF.begin();
    }

    auto end() const
    {
        return PDF.end();
    }

    std::vector<FAtom<ProbType, ValueType>> PDF;
};

template<typename ProbType, typename T, typename U>
TDist<ProbType, AddResultType<T, U>> operator+(const T&                  t,
                                               const TDist<ProbType, U>& du)
{
    return du.Transform([t](const U& u) { return t + u; });
}

template<typename ProbType, typename T, typename U>
TDist<ProbType, TimesResultType<T, U>> operator*(const T&                  t,
                                                 const TDist<ProbType, U>& du)
{
    return du.Transform([t](const U& u) { return t * u; });
}

template<typename X> using TDDist = TDist<double, X>;

template<typename ProbType = double, typename T>
std::ostream& operator<<(std::ostream& os, const FAtom<ProbType, T>& FAtom)
{
    os << "FAtom(Value: " << FAtom.Value << ", Prob: " << FAtom.Prob << ")";
    return os;
}

template<typename ProbType = double, typename T>
void SortAndCombine(std::vector<FAtom<ProbType, T>>& atoms)
{
    std::sort(atoms.begin(),
              atoms.end(),
              [](const FAtom<ProbType, T>& a, const FAtom<ProbType, T>& b)
              { return a.Value < b.Value; });

    std::vector<FAtom<ProbType, T>> combined;
    for (const auto& FAtom : atoms)
    {
        if (!combined.empty() && combined.back().Value == FAtom.Value)
        {
            combined.back().Prob += FAtom.Prob;
        }
        else
        {
            combined.push_back(FAtom);
        }
    }

    atoms = std::move(combined);
}
