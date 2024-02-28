#pragma once

#include <strsafe.h>
#include <map>

struct Rgb
{
	byte r, g, b;
};

inline bool operator==(Rgb const& lhs, Rgb const& rhs)
{
    return (lhs.r == rhs.r) && (lhs.g == rhs.g) && (lhs.b == rhs.b);
}

struct Hsv
{
	double h, s, v;
};

inline std::map<double, std::map<double, std::map<double, winrt::hstring>>> ColorNames;

struct ColorHelper
{
private:
    //winrt::Windows::UI::Color m_color{ 0, 0, 0, 0 };
    Rgb m_color{ 0, 0, 0 };

    template <typename TValue>
    double search_closest_map(const std::map<double, TValue>& map, double x) const
    {
        auto iter_geq = map.lower_bound(x);

        if (iter_geq == map.begin())
        {
            return iter_geq->first;
        }
        else if (iter_geq == map.end())
        {
            return std::prev(iter_geq)->first;
        }

        double a = std::prev(iter_geq)->first;
        double b = iter_geq->first;

        if (fabs(x - a) < fabs(x - b))
        {
            return a;
        }

        return b;
    }
    winrt::hstring get_closest_map(const std::map<double, std::map<double, std::map<double, winrt::hstring>>>& map, double h, double s, double v) const
    {
        if (map.empty()) return L"#NA";
        auto closestH = search_closest_map(map, h);
        auto& hp = map.at(closestH);

        if (hp.empty()) return L"#NA";
        auto closestS = search_closest_map(hp, s);
        auto& sp = hp.at(closestS);

        if (sp.empty()) return L"#NA";
        auto closestV = search_closest_map(sp, v);
        auto& name = sp.at(closestV);

        return name;
    }

public:
    ColorHelper() {};

    ColorHelper(winrt::hstring const& hex);
    ColorHelper(::Rgb const& rgb)
    {
        m_color = rgb;
    }
    ColorHelper(::Hsv const& hsv);
    ColorHelper(uint32_t const decimal);
    ColorHelper(winrt::Windows::UI::Color const& color);

    winrt::hstring Hex() const;
    Rgb Rgb() const { return m_color; }
    Hsv Hsv() const;
    uint32_t Decimal() const
    {
        auto const&& [r, g, b] = Rgb();
        const uint32_t dec = (r << 16) + (g << 8) + b;

        return dec;
    }
    winrt::Windows::UI::Color Color() const;

    operator winrt::hstring() const { return Hex(); }
    operator ::Rgb() const { return this->Rgb(); }
    operator ::Hsv() const { return this->Hsv(); }
    operator uint32_t() const { return Decimal(); }
    operator winrt::Windows::UI::Color() const { return Color(); }

    winrt::hstring ColorName(::Hsv const& hsv) const;
    winrt::hstring ColorName() const;
};