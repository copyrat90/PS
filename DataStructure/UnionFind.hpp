#ifndef __COPYRAT90_UNION_FIND_HPP__
#define __COPYRAT90_UNION_FIND_HPP__

#include <algorithm>
#include <vector>

namespace copyrat90
{

// 0-based idx.
// 노드 개수 N, 연산 개수 M이면 시간복잡도: O(Mlog*N).
// log*x 는 x=2^65536일 때 겨우 5이므로, 사실상 상수이니 전체 O(M)과 유사
class UnionFind
{
    using Num = int;

private:
    // 양수: 부모
    // 음수: 절댓값이 해당 set의 크기
    mutable std::vector<Num> parent_;
    // subtree의 높이(union by rank 최적화)
    std::vector<Num> rank_;

public:
    UnionFind(Num size) : parent_(size, -1), rank_(size, 0)
    {
    }

    Num find(Num n) const
    {
        // 자신이 루트이면 자신을 반환
        if (parent_[n] < 0)
            return n;

        // 루트를 찾아 재귀호출
        // 경로 압축 최적화
        parent_[n] = find(parent_[n]);
        return parent_[n];
    }

    // `false`: 이미 한 set 내부의 두 node면, 병합 수행하지 않고 건너뜀.
    // `true`: 병합 수행됨
    bool merge(Num n1, Num n2)
    {
        // 부모 찾아 대체
        n1 = find(n1);
        n2 = find(n2);
        // 같은 집합이면 merge 건너뜀
        if (n1 == n2)
            return false;

        // n1과 n2의 높이가 같은 경우, n2의 높이 한칸 증가
        if (rank_[n1] == rank_[n2])
            ++rank_[n2];
        // 다르면, n2가 높이가 n1보다 높도록 정규화
        else if (rank_[n1] > rank_[n2])
            std::swap(n1, n2);

        // set 원소 개수 더해주기
        parent_[n2] += parent_[n1];
        // set 합치기
        parent_[n1] = n2;

        return true;
    }

    // n번 노드가 속한 집합의 크기를 반환
    Num partSize(Num n) const
    {
        n = find(n);
        return -parent_[n];
    }

    // 전체 노드의 개수를 반환
    Num wholeSize() const
    {
        return parent_.size();
    }
};

} // namespace copyrat90

#endif
