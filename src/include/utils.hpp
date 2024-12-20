#include "standard.hpp"

namespace utils
{
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
                    i++;
                    if (!(str[i] & static_cast<char>(0x01)) && str[i] != static_cast<char>(0xb0))
                    {
                        str[i] |= static_cast<char>(0x01);
                    }
                    else if (str[i] == static_cast<char>(0xaf))
                    {
                        str[i] = static_cast<char>(0xb0);
                    }
                }
            }
            else
            {
                unsigned char byte = str[i];
                str[i] = std::tolower(byte);
            }
        }
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
            result = format("%.2LfKB", bytes);

            if (bytes > 1024)
            {
                bytes /= 1024;
                result = format("%.2LfMB", bytes);

                if (bytes > 1024)
                {
                    bytes /= 1024;
                    result = format("%.2LfGB", bytes);

                    if (bytes > 1024)
                    {
                        bytes /= 1024;
                        result = format("%.2LfTB", bytes);
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
