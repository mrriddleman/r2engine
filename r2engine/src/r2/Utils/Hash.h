//
//  Hash.hpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-06-19.
//
// Credit to 5gon12eder from https://stackoverflow.com/questions/34597260/stdhash-value-on-char-value-and-not-on-memory-address
#ifndef Hash_h
#define Hash_h

#include <string>

#define STRING_ID(str) r2::utils::Hash<const char*>{}(str)

namespace r2::utils
{
    constexpr size_t HashBytes(const void* data, size_t size);
    template <typename T>
    struct Hash
    {
        size_t operator()(const T& obj) const
        {
            auto hashfn = std::hash<T>{};
            return hashfn(obj);
        }
    };
    
    template <>
    struct Hash<std::string>
    {
        std::size_t
        operator()(const std::string& s) const noexcept
        {
            return HashBytes(s.data(), s.size());
        }
    };
    
    template <>
    struct Hash<const char*>
    {
        size_t operator()(const char* const s) const
        {
            return HashBytes(s, std::strlen(s));
        }
    };
    
    template <typename ResultT, ResultT OffsetBasis, ResultT Prime>
    class basic_fnv1a final
    {
        
        static_assert(std::is_unsigned<ResultT>::value, "need unsigned integer");
        
    public:
        
        using result_type = ResultT;
        
    private:
        
        result_type state_ {};
        
    public:
        
        constexpr
        basic_fnv1a() noexcept : state_ {OffsetBasis}
        {
        }
        
        constexpr void
        update(const void *const data, const std::size_t size) noexcept
        {
            const auto cdata = static_cast<const unsigned char *>(data);
            auto acc = this->state_;
            for (auto i = std::size_t {}; i < size; ++i)
            {
                const auto next = std::size_t {cdata[i]};
                acc = (acc ^ next) * Prime;
            }
            this->state_ = acc;
        }
        
        constexpr result_type
        digest() const noexcept
        {
            return this->state_;
        }
        
    };
    
    using fnv1a_32 = basic_fnv1a<std::uint32_t,
    0x811C9DC5,
    0x01000193>;
    
    using fnv1a_64 = basic_fnv1a<std::uint64_t,
    0xcbf29ce484222645ULL,
    0x00000100000001b3ULL>;
    
    template <std::size_t Bits>
    struct fnv1a;
    
    template <>
    struct fnv1a<32>
    {
        using type = fnv1a_32;
    };
    
    template <>
    struct fnv1a<64>
    {
        using type = fnv1a_64;
    };
    
    template <std::size_t Bits>
    using fnv1a_t = typename fnv1a<Bits>::type;
    
    constexpr std::size_t
    HashBytes(const void *const data, const std::size_t size)
    {
        auto hashfn = fnv1a_t<CHAR_BIT * sizeof(std::size_t)> {};
        hashfn.update(data, size);
        return hashfn.digest();
    }

    constexpr unsigned int HashBytes32(const void* const data, const std::size_t size)
    {
        auto hashfn = fnv1a_t<32>{};
        hashfn.update(data, size);
        return hashfn.digest();
    }
}

#endif /* Hash_h */
