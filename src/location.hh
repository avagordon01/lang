#include <algorithm>
#include <iostream>
#include <string>

namespace yy {

class position {
public:
    position(
        std::string* f = nullptr,
        unsigned l = 1u,
        unsigned c = 1u
    ):
        filename (f),
        line (l),
        column (c)
    {}

    void initialize(
        std::string* fn = nullptr,
        unsigned l = 1u,
        unsigned c = 1u
    ) {
        filename = fn;
        line = l;
        column = c;
    }

    void lines (int count = 1) {
        if (count) {
            column = 1u;
            line = add_ (line, count, 1);
        }
    }

    void columns (int count = 1) {
        column = add_ (column, count, 1);
    }

    std::string* filename;
    unsigned line;
    unsigned column;

private:
    static unsigned add_ (unsigned lhs, int rhs, int min) {
        return static_cast<unsigned>(
            std::max(min, static_cast<int>(lhs) + rhs)
        );
    }
};

inline position&
operator+=(position& res, int width) {
    res.columns (width);
    return res;
}

inline position
operator+(position res, int width) {
    return res += width;
}

inline position&
operator-=(position& res, int width) {
    return res += -width;
}

inline position
operator-(position res, int width) {
    return res -= width;
}

inline bool
operator==(const position& pos1, const position& pos2) {
    return (
        pos1.line == pos2.line &&
        pos1.column == pos2.column && (
        pos1.filename == pos2.filename || (
        pos1.filename && pos2.filename &&
        *pos1.filename == *pos2.filename))
    );
}

inline bool
operator!=(const position& pos1, const position& pos2) {
    return !(pos1 == pos2);
}

template <typename YYChar>
std::basic_ostream<YYChar>&
operator<<(std::basic_ostream<YYChar>& ostr, const position& pos) {
    if (pos.filename) {
        ostr << *pos.filename << ':';
    }
    return ostr << pos.line << '.' << pos.column;
}

class location {
public:
    location(
        const position& b,
        const position& e
    ):
        begin(b),
        end(e)
    {}

    location(const position& p = position ()): begin (p), end (p) {}

    location(
        std::string* f,
        unsigned l = 1u,
        unsigned c = 1u
    ):
        begin(f, l, c),
        end(f, l, c)
    {}

    void initialize(
        std::string* f = nullptr,
        unsigned l = 1u,
        unsigned c = 1u
    ) {
        begin.initialize (f, l, c);
        end = begin;
    }

public:
    void step() {
        begin = end;
    }

    void columns(int count = 1) {
        end += count;
    }

    void lines(int count = 1) {
        end.lines (count);
    }

public:
    position begin;
    position end;
};

inline location& operator+=(location& res, const location& end) {
    res.end = end.end;
    return res;
}

inline location operator+(location res, const location& end) {
    return res += end;
}

inline location& operator+=(location& res, int width) {
    res.columns (width);
    return res;
}

inline location operator+(location res, int width) {
    return res += width;
}

inline location& operator-=(location& res, int width) {
    return res += -width;
}

inline location operator-(location res, int width) {
    return res -= width;
}

inline bool
operator==(const location& loc1, const location& loc2) {
    return loc1.begin == loc2.begin && loc1.end == loc2.end;
}

inline bool
operator!=(const location& loc1, const location& loc2) {
    return !(loc1 == loc2);
}

template <typename YYChar>
std::basic_ostream<YYChar>&
operator<<(std::basic_ostream<YYChar>& ostr, const location& loc) {
    unsigned end_col = 0 < loc.end.column ? loc.end.column - 1 : 0;
    ostr << loc.begin;
    if (loc.end.filename &&
        (!loc.begin.filename || *loc.begin.filename != *loc.end.filename)
    ) {
        ostr << '-' << loc.end.filename << ':' << loc.end.line << '.' << end_col;
    } else if (loc.begin.line < loc.end.line) {
        ostr << '-' << loc.end.line << '.' << end_col;
    } else if (loc.begin.column < end_col) {
        ostr << '-' << end_col;
    }
    return ostr;
}

}
