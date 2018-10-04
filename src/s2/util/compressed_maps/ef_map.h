#pragma once

template <typename Key, typename Value>
class ef_map {
public:
    using key_container_type = std::vector<Key>;
    using value_container_type = std::vector<Value>;
    using key_type = typename Key;
    using value_type = typename Value;
    using size_type = typename std::size_t;
    using difference_type = typename std::ptrdiff_t;
    using reference = typename value_container_type::reference;
    using const_reference = typename value_container_type::const_reference;

protected:
    value_container_type m_values;
    ef_list m_ef;
    size_type m_size;
    size_type m_first_pos;
    size_type m_last_pos;
    bool m_frozen;
public:

    class const_iterator {

    };

    // Iterator routines.
    const_iterator begin() const { return const_iterator(m_values,m_ef,m_first_pos); }
    const_iterator end() const { return const_iterator(m_values,m_ef,m_last_pos); }

    const_iterator lower_bound(const key_type &key) const {
        size_type pos = m_ef.next_geq(key);
        return const_iterator(m_values,m_ef,pos);
    }

    // Utility routines.
    void clear() {
        m_frozen = false;
        m_size = 0;
        m_values.clear();

    }

    void freeze() {

    }

    void swap(ef_map &x) {

    }
    void verify() const {

    }

    // Size routines.
    size_type size() const { return m_size; }
    bool empty() const { return m_size == 0; }
    size_type bytes_used() const { return tree_.bytes_used(); }

    friend bool operator==(const ef_map &x, const ef_map &y) {
        if (x.size() != y.size()) return false;
        return std::equal(x.begin(), x.end(), y.begin());
    }

    friend bool operator!=(const ef_map &x, const ef_map &y) {
        return !(x == y);
    }

    ef_map() {};
};
