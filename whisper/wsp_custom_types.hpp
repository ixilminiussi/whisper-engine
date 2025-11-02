#ifndef WSP_CUSTOM_TYPES
#define WSP_CUSTOM_TYPES

#include <algorithm>
#include <stdexcept>
#include <vector>

namespace wsp // custom types
{

template <typename Key, typename Val> struct dictionary
{
    std::vector<std::pair<Key, Val>> data;

    dictionary() : data{} {};
    dictionary(std::vector<std::pair<Key, Val>> const &initial) : data{initial}
    {
        if (!std::is_default_constructible_v<Val>)
        {
            throw std::invalid_argument("wsp::dictionary keys and values must be default constructible");
        }
    }

    Val &operator[](Key const &key)
    {
        auto mappedInput = std::find_if(data.begin(), data.end(), [&](auto const &e) { return e.first == key; });

        if (mappedInput != data.end())
        {
            return mappedInput->second;
        }

        data.emplace_back(key, Val{});
        return std::find_if(data.begin(), data.end(), [&](auto const &e) { return e.first == key; })->second;
    }

    bool contains(Key const &key) const
    {
        auto mappedInput = std::find_if(data.begin(), data.end(), [&](auto const &e) { return e.first == key; });

        return mappedInput != data.end();
    }

    Key &from(Val const &val)
    {
        auto mappedInput = std::find_if(data.begin(), data.end(), [&](auto const &e) { return e.second == val; });

        if (mappedInput == data.end())
        {
            throw std::invalid_argument("wsp::dictionary::from called on inexistant value");
        }

        return mappedInput->first;
    }

    Key const &from(Val const &val) const
    {
        auto mappedInput = std::find_if(data.begin(), data.end(), [&](auto const &e) { return e.second == val; });

        if (mappedInput == data.end())
        {
            throw std::invalid_argument("wsp::dictionary::from called on inexistant value");
        }

        return mappedInput->first;
    }

    Val const &at(Key const &key) const
    {
        auto mappedInput = std::find_if(data.begin(), data.end(), [&](auto const &e) { return e.first == key; });

        if (mappedInput == data.end())
        {
            throw std::invalid_argument("wsp::dictionary::at called on inexistant key");
        }

        return mappedInput->second;
    }

    void clear()
    {
        data.clear();
    }

    auto begin()
    {
        return data.begin();
    }

    auto begin() const
    {
        return data.begin();
    }

    auto end()
    {
        return data.end();
    }

    auto end() const
    {
        return data.end();
    }
};

} // namespace wsp

#endif
