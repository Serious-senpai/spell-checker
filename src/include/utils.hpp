#pragma once

#include "standard.hpp"

namespace utils
{
    template <typename _Iterator>
    using _iterator_category_t = typename std::iterator_traits<_Iterator>::iterator_category;

    template <typename _InputIterator>
    using is_input_iterator_t = std::enable_if_t<std::is_convertible_v<_iterator_category_t<_InputIterator>, std::input_iterator_tag>, bool>;

    /**
     * @brief Format a string with C specifiers.
     * @see https://stackoverflow.com/a/26221725
     *
     * @tparam ...Args The format arguments
     * @param format The format string
     * @param args The arguments to format
     * @return The formatted string
     */
    template <typename... Args>
    std::string format(const std::string &format, const Args &...args)
    {
        int size_s = std::snprintf(nullptr, 0, format.c_str(), args...) + 1; // Extra space for '\0'
        if (size_s <= 0)
        {
            throw std::runtime_error("Error during formatting.");
        }
        auto size = static_cast<size_t>(size_s);
        std::unique_ptr<char[]> buf(new char[size]);
        std::snprintf(buf.get(), size, format.c_str(), args...);
        return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
    }

    /**
     * @brief Convert a string to lowercase.
     */
    void to_lower(std::string &str)
    {
        for (std::size_t i = 0; i < str.size(); i++)
        {
            if (str[i] & static_cast<char>(0x80))
            {
                if (str[i] == static_cast<char>(0xe1))
                {
                    i += 2;
                    str[i] |= static_cast<char>(0x01);
                }
                else if (str[i] == static_cast<char>(0xc3))
                {
                    str[++i] |= static_cast<char>(0x20);
                }
                else
                {
                    static const std::unordered_set<unsigned char> is_upper = {
                        0x82,
                        0x90,
                        0xa8,
                        0xa0,
                        0xaf,
                    };

                    i++;
                    if (is_upper.find(static_cast<unsigned char>(str[i])) != is_upper.end())
                    {
                        str[i]++;
                    }
                }
            }
            else
            {
                str[i] = std::tolower(str[i]);
            }
        }
    }

    /**
     * @brief Capitalize the character pointed to by `ptr`.
     *
     * @param ptr A pointer to the first byte of the character to capitalize.
     */
    void capitalize(char *ptr)
    {
        if (*ptr & static_cast<char>(0x80))
        {
            if (*ptr == static_cast<char>(0xe1))
            {
                *(ptr + 2) &= static_cast<char>(~0x01);
            }
            else if (*ptr == static_cast<char>(0xc3))
            {
                *++ptr &= static_cast<char>(~0x20);
            }
            else
            {
                static const std::unordered_set<unsigned char> is_lower = {
                    0x83,
                    0x91,
                    0xa9,
                    0xa1,
                    0xb0,
                };

                ptr++;
                if (is_lower.find(static_cast<unsigned char>(*ptr)) != is_lower.end())
                {
                    (*ptr)--;
                }
            }
        }
        else
        {
            *ptr = std::toupper(static_cast<char>(*ptr));
        }
    }

    bool is_utf8_char(const char *ptr)
    {
        return (*ptr & static_cast<char>(0xc0)) == static_cast<char>(0xc0) || (*ptr & static_cast<char>(0x80)) == 0;
    }

    /**
     * @brief Check if a character is an uppercase character.
     *
     * @param ptr A pointer to the first byte of the character to check.
     */
    bool is_upper(const char *ptr)
    {
        static const std::set<std::vector<unsigned char>> uppercase = {
            {0xc3, 0x80},       // À
            {0xc3, 0x81},       // Á
            {0xc3, 0x82},       // Â
            {0xc3, 0x83},       // Ã
            {0xc3, 0x88},       // È
            {0xc3, 0x89},       // É
            {0xc3, 0x8a},       // Ê
            {0xc3, 0x8c},       // Ì
            {0xc3, 0x8d},       // Í
            {0xc3, 0x92},       // Ò
            {0xc3, 0x93},       // Ó
            {0xc3, 0x94},       // Ô
            {0xc3, 0x95},       // Õ
            {0xc3, 0x99},       // Ù
            {0xc3, 0x9a},       // Ú
            {0xc3, 0x9d},       // Ý
            {0xc4, 0x82},       // Ă
            {0xc4, 0x90},       // Đ
            {0xc4, 0xa8},       // Ĩ
            {0xc5, 0xa8},       // Ũ
            {0xc6, 0xa0},       // Ơ
            {0xc6, 0xaf},       // Ư
            {0xe1, 0xba, 0xa0}, // Ạ
            {0xe1, 0xba, 0xa2}, // Ả
            {0xe1, 0xba, 0xa4}, // Ấ
            {0xe1, 0xba, 0xa6}, // Ầ
            {0xe1, 0xba, 0xa8}, // Ẩ
            {0xe1, 0xba, 0xaa}, // Ẫ
            {0xe1, 0xba, 0xac}, // Ậ
            {0xe1, 0xba, 0xae}, // Ắ
            {0xe1, 0xba, 0xb0}, // Ằ
            {0xe1, 0xba, 0xb2}, // Ẳ
            {0xe1, 0xba, 0xb4}, // Ẵ
            {0xe1, 0xba, 0xb6}, // Ặ
            {0xe1, 0xba, 0xb8}, // Ẹ
            {0xe1, 0xba, 0xba}, // Ẻ
            {0xe1, 0xba, 0xbc}, // Ẽ
            {0xe1, 0xba, 0xbe}, // Ế
            {0xe1, 0xbb, 0x80}, // Ề
            {0xe1, 0xbb, 0x82}, // Ể
            {0xe1, 0xbb, 0x84}, // Ễ
            {0xe1, 0xbb, 0x86}, // Ệ
            {0xe1, 0xbb, 0x88}, // Ỉ
            {0xe1, 0xbb, 0x8a}, // Ị
            {0xe1, 0xbb, 0x8c}, // Ọ
            {0xe1, 0xbb, 0x8e}, // Ỏ
            {0xe1, 0xbb, 0x90}, // Ố
            {0xe1, 0xbb, 0x92}, // Ồ
            {0xe1, 0xbb, 0x94}, // Ổ
            {0xe1, 0xbb, 0x96}, // Ỗ
            {0xe1, 0xbb, 0x98}, // Ộ
            {0xe1, 0xbb, 0x9a}, // Ớ
            {0xe1, 0xbb, 0x9c}, // Ờ
            {0xe1, 0xbb, 0x9e}, // Ở
            {0xe1, 0xbb, 0xa0}, // Ỡ
            {0xe1, 0xbb, 0xa2}, // Ợ
            {0xe1, 0xbb, 0xa4}, // Ụ
            {0xe1, 0xbb, 0xa6}, // Ủ
            {0xe1, 0xbb, 0xa8}, // Ứ
            {0xe1, 0xbb, 0xaa}, // Ừ
            {0xe1, 0xbb, 0xac}, // Ử
            {0xe1, 0xbb, 0xae}, // Ữ
            {0xe1, 0xbb, 0xb0}, // Ự
            {0xe1, 0xbb, 0xb2}, // Ỳ
            {0xe1, 0xbb, 0xb4}, // Ỵ
            {0xe1, 0xbb, 0xb6}, // Ỷ
            {0xe1, 0xbb, 0xb8}  // Ỹ
        };

        const auto first_byte = static_cast<unsigned char>(ptr[0]);

        // Check for single-byte ASCII uppercase letters first
        if (std::isupper(first_byte))
        {
            return true;
        }

        // Check for 2-byte uppercase characters
        std::vector<unsigned char> span = {first_byte, static_cast<unsigned char>(ptr[1])};
        if (uppercase.find(span) != uppercase.end())
        {
            return true;
        }

        span.push_back(static_cast<unsigned char>(ptr[2]));
        return uppercase.find(span) != uppercase.end();
    }

    /**
     * @brief Formatter for printing memory sizes, e.g. 1024 bytes -> `1.00KB`
     *
     * @param bytes The number of bytes to format, e.g. 1024
     * @return The formatted string, e.g. `1.00KB`
     */
    std::string memory_size(long double bytes)
    {
        std::string result = format("%.2LfB", bytes);
        if (bytes > 1024)
        {
            bytes /= 1024;
            result = format("%.2LfKiB", bytes);

            if (bytes > 1024)
            {
                bytes /= 1024;
                result = format("%.2LfMiB", bytes);

                if (bytes > 1024)
                {
                    bytes /= 1024;
                    result = format("%.2LfGiB", bytes);

                    if (bytes > 1024)
                    {
                        bytes /= 1024;
                        result = format("%.2LfTiB", bytes);
                    }
                }
            }
        }

        return result;
    }

    long long get_file_size(const std::string &filename)
    {
        struct stat64 stat_buf;
        int rc = stat64(filename.c_str(), &stat_buf);
        return rc == 0 ? stat_buf.st_size : -1;
    }

    template <typename T, std::enable_if_t<std::is_arithmetic_v<T>, bool> = true>
    T abs(const T &value)
    {
        return value > 0 ? value : -value;
    }
}
