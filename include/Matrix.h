#include <iostream>
#include <vector>
#include <deque>
#include <set>
#include <cassert>

template <typename P> using SparseVector = std::vector<std::pair<int, P>>;
template <typename P> using TMatrix = std::vector<SparseVector<P>>;

template <typename P> std::vector<int> SolveOrder(const TMatrix<P>& Matrix)
{
    std::vector<int> Order;
    int              Dim = Matrix.size();

    std::set<int> Absorbers;
    for (int RowNumber = 0; RowNumber < Dim; ++RowNumber)
    {
        // May need a better test
        if (Matrix[RowNumber].size() == 1)
        {
            Absorbers.insert(RowNumber);
        }
    }

    int Target = 0;
    Order.clear();

    std::vector<bool> Solved(Dim, false);

    std::deque<int> ToSolve;
    ToSolve.push_back(0);

    // XXX Temp!

    while (ToSolve.size() > 0)
    {
        int Next = ToSolve.front();
        ToSolve.pop_front();

        bool ReadyToSolve = true;
        for (auto [Label, Prob] : Matrix[Next])
        {
            if (Label == Target)
            {
            }
            else if (Absorbers.contains(Label))
            {
            }
            else if (Label == Next)
            {
            }
            else
            {
                if (!Solved[Label])
                {
                    if (ReadyToSolve)
                    {
                        ToSolve.push_front(Next);
                    }
                    ReadyToSolve = false;
                    ToSolve.push_front(Label);
                    ReadyToSolve = false;
                }
                else
                {
                }
            }
        }
        if (ReadyToSolve)
        {
            assert(!Absorbers.contains(Next));
            Order.push_back(Next);
            Solved[Next] = true;
        }
    }
    std::cout << "IOrder: ";
    for (auto K : Order)
    {
        std::cout << K << ' ';
    }
    std::cout << std::endl;

    return Order;
}

template <typename P>
std::vector<P> Solve(const TMatrix<P>& Matrix, const std::vector<P>& Init)
{
    //SolveOrder(Matrix);
    //exit(1);

    int Dim = Matrix.size();

    std::vector<P> Result(Dim, 0);

    // Find absorbers
    std::set<int> Absorbers;
    for (int RowNumber = 0; RowNumber < Dim; ++RowNumber)
    {
        // May need a better test
        if (Matrix[RowNumber].size() == 1)
        {
            Absorbers.insert(RowNumber);
        }
    }
    for (auto E : Absorbers)
    {
        // std::cout << "Absorber: " << E << std::endl;
    }

    int Count = 0;
    for (int Target = 0; Target < Dim; ++Target)
    {
        std::cout << "Target = " << Target << std::endl;
        if (!Absorbers.contains(Target))
        {
            continue;
        }

        std::vector<P>    V(Dim, 0);
        std::vector<bool> Solved(Dim, false);

        std::deque<int> ToSolve;
        // std::set<bool> Visited;
        ToSolve.push_back(0);

        // XXX Temp!

        while (ToSolve.size() > 0)
        {
            int Next = ToSolve.front();
            // std::cout << "Trying to solve for " << Next << std::endl;
            // if (Visited.contains(Next))
            //{
            //     std::cerr << "Revisiting " << Next << std::endl;
            //     exit(1);
            // }
            // Visited.insert(Next);
            ToSolve.pop_front();
            if (Solved[Next])
            {
                continue;
            }

            bool ReadyToSolve = true;
            P    Total = 0;
            P    Diag = 1;
            for (auto [Label, Prob] : Matrix[Next])
            {
                if (Prob == 0)
                {
                    continue;
                }
                if (Label == Target)
                {
                    // std::cout << "Target Label " << Label
                    //           << " Total = " << Total << " + " << Next << " =
                    //           "
                    //           << Total + Prob << std::endl;
                    Total += Prob;
                    ++Count;
                }
                else if (Absorbers.contains(Label))
                {
                }
                else if (Label == Next)
                {
                    Diag -= Prob;
                }
                else
                {
                    if (!Solved[Label])
                    {
                        // Solving for `Next` requires solving for `Label`
                        // first so we have to delay it.
                        if (ReadyToSolve)
                        {
                            ToSolve.push_front(Next);
                        }
                        ReadyToSolve = false;
                        ToSolve.push_front(Label);
                        // std::cout << "Pushing " << Label << std::endl;
                        ReadyToSolve = false;
                    }
                    else
                    {
                        // std::cout << "Label " << Label << " Total = " <<
                        // Total
                        //           << " + " << Prob * V[Label] << " = "
                        //           << Total + Prob * V[Label] << std::endl;
                        Total += Prob * V[Label];
                        ++Count;
                    }
                }
            }
            if (ReadyToSolve)
            {
                // std::cout << "Ready to solve for " << Next << std::endl;
                if (Diag == 0)
                {
                    assert(false);
                    // std::cout << "Diag = 0" << std::endl;
                    V[Next] = 1;
                }
                else
                {
                    // The variables we're solving for shouldn't include
                    // absorbers
                    assert(!Absorbers.contains(Next));
                    // std::cout << "Total = " << Total << " Diag = " << Diag
                    //           << std::endl;
                    assert(!Solved[Next]);
                    V[Next] = Total / Diag;
                    // std::cout << "Solved for " << Next << " as " << Total
                    //           << " / " << Diag << " = " << V[Next] <<
                    //           std::endl;
                }
                Solved[Next] = true;
            }
            else
            {
                // std::cout << "Not ready for " << Next << std::endl;
            }
        }
        // std::cout << "V = ";
        int I = 0;
        for (auto X : V)
        {
            Result[Target] += Init[I] * X;
            // std::cout << X << ' ';
            ++I;
        }
        // std::cout << std::endl;
    }
    std::cout << "Count = " << Count << std::endl;
    for (auto I : Absorbers)
    {
        Result[I] += Init[I];
    }

    return Result;
}

#if 0
int main()
{
    #if 1
    TMatrix<double> M{
        SparseVector<double>{ { 0, 1. / 2 }, { 1, 3. / 8 }, { 2, 1. / 8 } },
        SparseVector<double>{ { 1, 1. / 4 }, { 2, 1. / 4 }, { 3, 1. / 2 } },
        SparseVector<double>{ { 2, 2. / 3 }, { 4, 1. / 3 } },
        SparseVector<double>{ { 3, 1. } },
        SparseVector<double>{ { 4, 1. } }
    };
    #else
    TMatrix<double> M{
        SparseVector<double>{ { 0, 1. / 2 }, { 1, 3. / 8 }, { 3, 1. / 8 } },
        SparseVector<double>{ { 1, 1. / 4 }, { 3, 1. / 4 }, { 2, 1. / 2 } },
        SparseVector<double>{ { 2, 1. } },
        SparseVector<double>{ { 3, 2. / 3 }, { 4, 1. / 3 } },
        SparseVector<double>{ { 4, 1. } }
    };
    #endif

    std::vector<double> Init{ 1., 0., 0., 0., 0. };
    auto                V = Solve(M, Init);

    for (int i = 0; i < V.size(); ++i)
    {
        std::cout << V[i] << std::endl;
    }
}
#endif
