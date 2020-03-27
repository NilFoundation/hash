//---------------------------------------------------------------------------//
// Copyright (c) 2018-2019 Nil Foundation AG
// Copyright (c) 2020 Nikita Kaskov <nbering@nil.foundation>
// Copyright (c) 2020 Alexander Sokolov <asokolov@nil.foundation>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//---------------------------------------------------------------------------//

#ifndef CRYPTO3_INJECT_HASH_HPP
#define CRYPTO3_INJECT_HASH_HPP


#include <nil/crypto3/detail/stream_endian.hpp>
#include <nil/crypto3/detail/basic_functions.hpp>
#include <nil/crypto3/detail/unbounded_shift.hpp>

namespace nil {
    namespace crypto3 {
        namespace detail {
                
                template<typename Endianness, std::size_t WordBits, std::size_t BlockWords, std::size_t BlockBits>
                struct word_injector;

                template<int UnitBits, std::size_t WordBits, std::size_t BlockWords, std::size_t BlockBits>
                struct word_injector<stream_endian::big_unit_big_bit<UnitBits>, WordBits, BlockWords, BlockBits> : public basic_functions<WordBits>{

                    constexpr static const std::size_t word_bits = basic_functions<WordBits>::word_bits;
                    typedef typename basic_functions<WordBits>::word_type word_type;

                    typedef std::array<word_type, BlockWords> block_type;

                    static void inject(word_type w, std::size_t word_seen, block_type &b, std::size_t &block_seen) {
                        //Insert word_seen-bit part of word into the block b according to endianness

                        // Check whether we fall out of the block
                        if (block_seen + word_seen <= BlockBits) {
                            std::size_t last_word_ind = block_seen / word_bits;
                            std::size_t last_word_seen = block_seen % word_bits;

                            // Remove garbage
                            w &= high_bits<word_type, word_bits>(~word_type(), word_seen);
                            b[last_word_ind] &= high_bits<word_type, word_bits>(~word_type(), last_word_seen);

                            // Add significant word bits to block word
                            b[last_word_ind] |= (w >> last_word_seen);

                            // If we fall out of the block word, push the remainder of element to the next block word
                            if (last_word_seen + word_seen > word_bits)
                                b[last_word_ind + 1] = w << (word_bits - last_word_seen);

                            block_seen += word_seen;
                        }
                    }
                };

                template<int UnitBits, std::size_t WordBits, std::size_t BlockWords, std::size_t BlockBits>
                struct word_injector<stream_endian::little_unit_big_bit<UnitBits>, WordBits, BlockWords, BlockBits> : public basic_functions<WordBits>{

                    constexpr static const std::size_t word_bits = basic_functions<WordBits>::word_bits;
                    typedef typename basic_functions<WordBits>::word_type word_type;

                    typedef std::array<word_type, BlockWords> block_type;

                    static void inject(word_type w, std::size_t word_seen, block_type &b, std::size_t block_seen) {
                        //Insert word_seen-bit part of word into the block b according to endianness
                        
                        // Check whether we fall out of the block
                        if (block_seen + word_seen <= BlockBits) {
                            std::size_t last_word_ind = block_seen / word_bits;
                            std::size_t last_word_seen = block_seen % word_bits;

                            // Remove garbage
                            std::size_t w_rem = word_seen % UnitBits;
                            std::size_t w_unit_bits = word_seen - w_rem;
                            word_type mask = low_bits<word_type, word_bits>(~word_type(), w_unit_bits) 
                            | (low_bits<word_type, word_bits>(~word_type(), w_rem) << (w_unit_bits + UnitBits - w_rem)); 
                            w &= mask; 
                            std::size_t b_rem = last_word_seen % UnitBits;
                            std::size_t b_unit_bits = last_word_seen - b_rem;
                            mask = low_bits<word_type, word_bits>(~word_type(), b_unit_bits) 
                            | (low_bits<word_type, word_bits>(~word_type(), b_rem) << (b_unit_bits + UnitBits - b_rem));
                            b[last_word_ind] &= mask;

                            // Split and combine parts of unit values
                            std::size_t sz[2] = {UnitBits - b_rem, b_rem};
                            word_type masks[2] = {low_bits<word_type, word_bits>(~word_type(), UnitBits - b_rem) << b_rem, 
                                low_bits<word_type, word_bits>(~word_type(), b_rem)};
                            std::size_t bw_space = word_bits - last_word_seen;
                            word_type w_split = 0;
                            std::size_t sz_ind = 0;
                              
                            while (bw_space && w) {
                                w_split |= (!sz_ind ? ((w & masks[0]) >> b_rem) : ((w & masks[1]) << (UnitBits + sz[0]))); 
                                bw_space -= sz[sz_ind];
                                w &= ~masks[sz_ind];
                                masks[sz_ind] <<= UnitBits;
                                sz_ind = 1 - sz_ind;
                            }

                            // Add significant word bits to block word
                            b[last_word_ind] |= w_split << b_unit_bits;

                            // If we fall out of the block word, push the remainder of element to the next block word
                            if (w) {
                                w >>= (word_bits - b_unit_bits - UnitBits);
                                w_split = 0;
                                masks[0] = low_bits<word_type, word_bits>(~word_type(), UnitBits - b_rem) << b_rem; 
                                masks[1] = low_bits<word_type, word_bits>(~word_type(), b_rem);

                                while (w) {
                                    w_split |= (!sz_ind ? ((w & masks[0]) >> b_rem) : ((w & masks[1]) << (UnitBits + sz[0]))); 
                                    w &= ~masks[sz_ind];
                                    masks[sz_ind] <<= UnitBits;
                                    sz_ind = 1 - sz_ind;
                                }

                                b[last_word_ind + 1] = w_split >> UnitBits;
                            }

                            block_seen += word_seen;
                        }
                    }
                };

                template<int UnitBits, std::size_t WordBits, std::size_t BlockWords, std::size_t BlockBits>
                struct word_injector<stream_endian::big_unit_little_bit<UnitBits>, WordBits, BlockWords, BlockBits> : public basic_functions<WordBits>{

                    constexpr static const std::size_t word_bits = basic_functions<WordBits>::word_bits;
                    typedef typename basic_functions<WordBits>::word_type word_type;

                    typedef std::array<word_type, BlockWords> block_type;

                    static void inject(word_type w, std::size_t word_seen, block_type &b, std::size_t block_seen) {
                        //Insert word_seen-bit part of word into the block b according to endianness
                        
                        // Check whether we fall out of the block
                        if (block_seen + word_seen <= BlockBits) {
                            std::size_t last_word_ind = block_seen / word_bits;
                            std::size_t last_word_seen = block_seen % word_bits;

                            // Remove garbage
                            std::size_t w_rem = word_seen % UnitBits;
                            std::size_t w_unit_bits = word_seen - w_rem;
                            word_type mask = high_bits<word_type, word_bits>(~word_type(), w_unit_bits) 
                            | (high_bits<word_type, word_bits>(~word_type(), w_rem) >> (w_unit_bits + UnitBits - w_rem)); 
                            w &= mask; 
                            std::size_t b_rem = last_word_seen % UnitBits;
                            std::size_t b_unit_bits = last_word_seen - b_rem;
                            mask = high_bits<word_type, word_bits>(~word_type(), b_unit_bits) 
                            | (high_bits<word_type, word_bits>(~word_type(), b_rem) >> (b_unit_bits + UnitBits - b_rem));
                            b[last_word_ind] &= mask;

                            // Split and combine parts of unit values 
                            std::size_t sz[2] = {UnitBits - b_rem, b_rem};
                            word_type masks[2] = {high_bits<word_type, word_bits>(~word_type(), UnitBits - b_rem) >> b_rem, 
                                high_bits<word_type, word_bits>(~word_type(), b_rem)};
                            std::size_t bw_space = word_bits - last_word_seen;
                            word_type w_split = 0;
                            std::size_t sz_ind = 0;
                             
                            while (bw_space && w) {
                                w_split |= (!sz_ind ? ((w & masks[0]) << b_rem) : ((w & masks[1]) >> (UnitBits + sz[0]))); 
                                bw_space -= sz[sz_ind];
                                w &= ~masks[sz_ind];
                                masks[sz_ind] >>= UnitBits;
                                sz_ind = 1 - sz_ind;
                            }

                            // Add significant word bits to block word
                            b[last_word_ind] |= w_split >> b_unit_bits;

                            // If we fall out of the block word, push the remainder of element to the next block word
                            if (w) {
                                w <<= (word_bits - b_unit_bits - UnitBits);
                                w_split = 0;
                                masks[0] = high_bits<word_type, word_bits>(~word_type(), UnitBits - b_rem) >> b_rem; 
                                masks[1] = high_bits<word_type, word_bits>(~word_type(), b_rem);

                                while (w) {
                                    w_split |= (!sz_ind ? ((w & masks[0]) << b_rem) : ((w & masks[1]) >> (UnitBits + sz[0]))); 
                                    w &= ~masks[sz_ind];
                                    masks[sz_ind] >>= UnitBits;
                                    sz_ind = 1 - sz_ind;
                                }

                                b[last_word_ind + 1] = w_split << UnitBits;
                            }
                            block_seen += word_seen;
                        }
                    }
                };

                template<int UnitBits, std::size_t WordBits, std::size_t BlockWords, std::size_t BlockBits>
                struct word_injector<stream_endian::little_unit_little_bit<UnitBits>, WordBits, BlockWords, BlockBits> : public basic_functions<WordBits>{

                    constexpr static const std::size_t word_bits = basic_functions<WordBits>::word_bits;
                    typedef typename basic_functions<WordBits>::word_type word_type;

                    typedef std::array<word_type, BlockWords> block_type;

                    static void inject(word_type w, std::size_t word_seen, block_type &b, std::size_t block_seen) {
                        //Insert word_seen-bit part of word into the block b according to endianness
                        
                        // Check whether we fall out of the block
                        if (block_seen + word_seen <= BlockBits) {
                            std::size_t last_word_ind = block_seen / word_bits;
                            std::size_t last_word_seen = block_seen % word_bits;

                            // Remove garbage
                            w &= low_bits<word_type, word_bits>(~word_type(), word_seen);
                            b[last_word_ind] &= low_bits<word_type, word_bits>(~word_type(), last_word_seen);

                            // Add significant word bits to block word
                            b[last_word_ind] |= (w << last_word_seen);

                            // If we fall out of the block word, push the remainder of element to the next block word
                            if (last_word_seen + word_seen > word_bits)
                                b[last_word_ind + 1] = w >> (word_bits - last_word_seen);

                            block_seen += word_seen;
                        }
                    }
                };

                template<typename Endianness, std::size_t WordBits, std::size_t BlockWords, std::size_t BlockBits>
                struct injector: word_injector<Endianness, WordBits, BlockWords, BlockBits>{

                    constexpr static const std::size_t word_bits = basic_functions<WordBits>::word_bits;
                    typedef typename basic_functions<WordBits>::word_type word_type;

                    typedef std::array<word_type, BlockWords> block_type;

                    static void inject(const block_type &b_src, std::size_t b_src_seen, block_type &b_dst, std::size_t &b_dst_seen) {
                        //Insert word_seen-bit part of word into the block b according to endianness

                        // Check whether we fall out of the block
                        if (b_src_seen + b_dst_seen <= BlockBits) {
                            
                            for (std::size_t i = 0; i< (b_src_seen / word_bits); i++){
                                inject(b_src[i], word_bits, b_dst, b_dst_seen);
                            }

                            if(b_src_seen%word_bits){
                                inject(b_src[b_src_seen / word_bits], b_src_seen%word_bits, b_dst, b_dst_seen);
                            }

                        }
                    }

                    static void inject(word_type w, std::size_t word_seen, block_type &b, std::size_t &block_seen){
                        word_injector<Endianness, WordBits, BlockWords, BlockBits>::inject(w, word_seen, b, block_seen);
                    }
                };
                    
        }        // namespace detail
    }            // namespace crypto3
}    // namespace nil

#endif    // CRYPTO3_INJECT_HASH_HPP