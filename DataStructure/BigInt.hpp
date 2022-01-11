// * Naive C++20 BigInt 클래스
// * https://github.com/copyrat90/PS

// * 최초 작성자: copyrat90
// * 추가 기여자:
// * 최종 수정일: 2022-01-04

// * 사칙연산 중 나눗셈은 아직 미구현(음수의 나눗셈 너무 구현이 귀찮음)
// * MIT/Expat 라이선스로 배포됩니다.

// MIT License
//
// Copyright 2022 copyrat90 <copyrat90@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef __COPYRAT90_BIG_INT_HPP__
#define __COPYRAT90_BIG_INT_HPP__

// C++20 이전 버전에서는 작동하지 않음
static_assert(__cplusplus >= 202002L, "copyrat90::BigInt requires C++20 or later.\n"
                                      "Try --std=c++20 compile flag.\n"
                                      "If it doesn't work, you need to upgrade your compiler.\n");

#include <cassert>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace copyrat90
{

#ifdef COPYRAT90_DEBUG
constexpr bool copyrat90_debug = true;
#else
constexpr bool copyrat90_debug = false;
#endif

class BigInt
{
    // 곱셈 중간 결과에서 오버플로우 발생하는 걸
    // 막기 위해 넉넉하게 잡아준다. (낭비같지만...)
    using DigitType = std::int16_t;

private:
    // true: 음수, false: 양수
    bool signBit_;

    // 배열의 각 요소를 0~9 범위의 숫자로 저장
    // idx=0: 1의 자리, idx=1: 10의 자리...
    std::vector<DigitType> digits_;

public:
    // 기본값 0으로 생성
    BigInt() : BigInt(0)
    {
    }

    // 숫자로 이루어진 문자열을 받는 생성자
    BigInt(std::string_view str)
    {
        // 문자열 길이가 1 이상이어야 정상적인 BigInt
        if (str.size() == 0)
            throw std::invalid_argument("Invalid length: str.size() == 0");

        // 부호(음수/양수) 처리 및 첫번째 문자 검증
        if (str[0] == '+' || str[0] == '-' || '0' <= str[0] && str[0] <= '9')
        {
            signBit_ = (str[0] == '-');
            if (str[0] == '+' || str[0] == '-')
                str = str.substr(1);
        }
        else
            throw std::invalid_argument("Invalid character in str");

        // 부호 뗀 문자열 길이가 1 이상이어야 정상적인 BigInt
        if (str.size() == 0)
            throw std::invalid_argument("Invalid length: str.size() == 0");

        // 부호 제외 문자열이 숫자를 나타내는지 검증
        for (auto ch : str)
            if (!('0' <= ch && ch <= '9'))
                throw std::invalid_argument("Invalid character in str");

        // 숫자 배열 digits_ 초기화
        digits_.reserve(str.size());
        for (int i = str.size() - 1; i >= 0; --i)
            digits_.push_back(str[i] - '0');

        removeLeadingZero();
        normalizeZero();
    }

    // str이 아래 정수 타입 생성자로 추론되어 컴파일 오류나는 걸 방지
    BigInt(const char* str) : BigInt(std::string_view(str))
    {
    }
    BigInt(const std::string& str) : BigInt(std::string_view(str))
    {
    }

    // 모든 정수 타입(e.g. unsigned, int, long long 등) 받는 생성자
    template <typename STD_INT>
    requires std::is_integral_v<STD_INT> BigInt(STD_INT number)
    {
        constexpr std::size_t bits = sizeof(STD_INT) * 8;

        // 부호 처리
        signBit_ = number < 0;

        // number가 음수 최댓값인지 여부를 저장
        // 음수 최댓값이 양수 최댓값보다 범위가 1 크기 때문에,
        // abs 하기 전에 따로 예외 처리가 필요.
        bool isNumberMaxNegative = false;

        if (number < 0)
        {
            // 사실 부호 비트를 덮어쓸때까지 LShift 하는 것은 UB이지만...
            // 마땅히 대안이 안보인다...
            constexpr STD_INT MAX_NEGATIVE = 1 << (bits - 1);
            isNumberMaxNegative = (number == MAX_NEGATIVE);
            // 음수 최댓값일 경우, 일단 +1 해서 최댓값보다 작게 만들어놓는다. (abs 예외처리)
            if (isNumberMaxNegative)
                ++number;

            // number의 절댓값을 취한다.
            number = -number;
        }
        // 0인 경우 따로 처리
        else if (number == 0)
        {
            digits_.push_back(0);
            return;
        }

        // 숫자 배열 digits_ 초기화
        while (number > 0)
        {
            digits_.push_back(number % 10);
            number /= 10;
        }

        // 음수 최댓값이었으면, +1 을 추가로 해 준다. (abs 예외처리)
        if (isNumberMaxNegative)
            ++digits_[0];
    }

    // 복사 및 이동
    BigInt(const BigInt&) = default;
    BigInt& operator=(const BigInt&) = default;
    BigInt(BigInt&& other) : signBit_(other.signBit_), digits_(std::move(other.digits_))
    {
    }
    BigInt& operator=(BigInt&& other)
    {
        signBit_ = other.signBit_;
        digits_ = std::move(other.digits_);
        return *this;
    }

    // 절댓값 반환하는 함수
    BigInt abs() const
    {
        BigInt absNum(*this);
        absNum.signBit_ = false;
        return absNum;
    }

    // 비교 연산
    auto operator<=>(const BigInt& other) const
    {
        if (this->signBit_ ^ other.signBit_)
        {
            // *this=음수, other=양수
            if (this->signBit_)
                return std::strong_ordering::less;
            // *this=양수, other=음수
            else
                return std::strong_ordering::greater;
        }
        // 둘 다 음수
        else if (this->signBit_)
        {
            return other.abs() <=> this->abs();
        }
        // 둘 다 양수
        else
        {
            if (this->digits_.size() < other.digits_.size())
                return std::strong_ordering::less;
            if (this->digits_.size() > other.digits_.size())
                return std::strong_ordering::greater;
            for (int i = digits_.size() - 1; i >= 0; --i)
                if (this->digits_[i] < other.digits_[i])
                    return std::strong_ordering::less;
                else if (this->digits_[i] > other.digits_[i])
                    return std::strong_ordering::greater;
        }
        return std::strong_ordering::equal;
    }

    bool operator==(const BigInt& other) const
    {
        if (this->signBit_ != other.signBit_)
            return false;
        if (this->digits_.size() != other.digits_.size())
            return false;
        for (int i = 0; i < digits_.size(); ++i)
            if (this->digits_[i] != other.digits_[i])
                return false;
        return true;
    }

    // 부호 반전 연산자
    BigInt operator-() const
    {
        BigInt neg(*this);
        neg.signBit_ = !neg.signBit_;
        return neg;
    }

    // 사칙연산

    BigInt operator+(const BigInt& other) const
    {
        BigInt sum(*this);
        // 두 숫자의 부호가 다를 때
        if (sum.signBit_ ^ other.signBit_)
        {
            // sum=음수, other=양수
            if (sum.signBit_)
            {
                if (other >= sum.abs())
                {
                    BigInt temp(other);
                    temp.subAbs(sum);
                    sum = std::move(temp);
                }
                else
                {
                    sum.subAbs(other);
                }
            }
            // sum=양수, other=음수
            else
            {
                if (sum >= other.abs())
                {
                    sum.subAbs(other);
                }
                else
                {
                    BigInt temp(other);
                    temp.subAbs(sum);
                    sum = std::move(temp);
                }
            }
        }
        // 두 숫자의 부호가 같을 때
        else
        {
            sum.addAbs(other);
        }
        return sum;
    }

    BigInt operator-(const BigInt& other) const
    {
        return *this + (-other);
    }

    BigInt operator*(const BigInt& other) const
    {
        // WIP
        BigInt result(0);
        // 부호 처리
        result.signBit_ = this->signBit_ ^ other.signBit_;

        // 미리 곱셈 공간 확장
        result.digits_.resize(this->digits_.size() + other.digits_.size() - 1, 0);

        // 초등학교 곱셈 알고리즘
        for (int t = 0; t < this->digits_.size(); ++t)
            for (int o = 0; o < other.digits_.size(); ++o)
                result.digits_[t + o] += this->digits_[t] * other.digits_[o];

        // 정규화
        result.carryUp();
        result.removeLeadingZero();
        result.normalizeZero();

        return result;
    }

    BigInt& operator+=(const BigInt& other)
    {
        return (*this = *this + other);
    }

    BigInt& operator-=(const BigInt& other)
    {
        return (*this = *this - other);
    }

    BigInt operator*=(const BigInt& other)
    {
        return (*this = *this * other);
    }

private:
    // 부호 고려하지 않은 덧셈
    void addAbs(const BigInt& other)
    {
        // other가 자릿수가 더 많을 경우, 같은 크기로 자릿수 확장
        if (other.digits_.size() > this->digits_.size())
            this->digits_.resize(other.digits_.size());

        // carry 처리 없는 단순 배열 덧셈
        for (int i = 0; i < other.digits_.size(); ++i)
            this->digits_[i] += other.digits_[i];

        // carry 처리
        carryUp();
    }

    // 부호 고려하지 않은 뺄셈
    void subAbs(const BigInt& other)
    {
        if constexpr (copyrat90_debug)
            assert(this->abs() >= other.abs());

        // carry 처리 없는 단순 배열 뺄셈
        for (int i = other.digits_.size() - 1; i >= 0; --i)
            this->digits_[i] -= other.digits_[i];

        // carry 처리
        carryDown();
    }

    // -0 => +0
    void normalizeZero()
    {
        if (digits_.size() == 1 && digits_[0] == 0)
        {
            signBit_ = false;
        }
    }

    // 덧셈 후 발생한 carry를 옮겨 digits_를 0~9 범위로 정규화
    void carryUp()
    {
        for (int i = 0; i < digits_.size(); ++i)
        {
            const DigitType carry = digits_[i] / 10;
            // 자릿수 공간 부족하면 추가 확장
            if (carry && (i == digits_.size() - 1))
                digits_.push_back(0);

            digits_[i + 1] += carry;
            digits_[i] %= 10;
        }
    }

    // 뺄셈 후 발생한 음수를 위 자릿수를 빌려와 0~9 범위로 정규화
    // 반드시 digits_ 전체가 양수여야 정상 작동
    // (절댓값 큰 값에서 절댓값 작은 값을 뺀 경우만 정규화 가능)
    void carryDown()
    {
        for (int i = 0; i < digits_.size() - 1; ++i)
        {
            while (digits_[i] < 0)
            {
                digits_[i + 1]--;
                digits_[i] += 10;
            }
        }
        removeLeadingZero();
        normalizeZero();
    }

    // 숫자 앞에 붙은 0을 제거
    void removeLeadingZero()
    {
        // 1의 자리 0은 남겨야 하므로, idx >= 1 까지만 시행
        for (int i = digits_.size() - 1; i >= 1; --i)
        {
            if (digits_[i] != 0)
                return;
            else
                digits_.pop_back();
        }
    }

    friend std::ostream& operator<<(std::ostream& os, const BigInt& num);
};

inline std::ostream& operator<<(std::ostream& os, const BigInt& num)
{
    if (num.signBit_)
        os << '-';
    for (int i = num.digits_.size() - 1; i >= 0; --i)
        os << (int)num.digits_[i];
    return os;
}

inline std::istream& operator>>(std::istream& is, BigInt& num)
{
    std::string str;
    is >> str;
    num = BigInt(str);
    return is;
}

} // namespace copyrat90

#endif
