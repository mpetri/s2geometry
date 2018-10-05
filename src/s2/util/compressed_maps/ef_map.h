#pragma once

#include "s2/util/compressed_maps/ds2i/global_parameters.hpp"
#include "s2/util/compressed_maps/ds2i/compact_elias_fano.hpp"

template <typename key_type, typename value_type>
class ef_map {
public:
    using value_container_type = std::vector<value_type>;
    using size_type = std::size_t;
    using const_reference = typename value_container_type::const_reference;

protected:
    value_container_type m_values;
    succinct::bit_vector m_ef;
    size_type m_universe;
    quasi_succinct::global_parameters m_params;
public:

    struct const_iterator {
        typedef const_iterator self_type;
        typedef std::pair<key_type,value_type> pair_type;
        typedef pair_type& reference;
        typedef pair_type* pointer;
        typedef int difference_type;
        typedef std::forward_iterator_tag iterator_category;

        quasi_succinct::compact_elias_fano::enumerator m_enum;
        value_container_type& m_values;
        pair_type m_cur_value;

        const_iterator() = delete;

        const_iterator(const value_container_type& v,
                       const succinct::bit_vector& b,
                       size_type pos,
                       size_type universe,
                       size_type n,quasi_succinct::global_parameters const& params)
            : m_enum(v), m_enum(b,0,universe,n,params)
        {
            if(pos != n) m_enum.move(pos);
        }

        self_type operator++(int junk) {
            m_enum.next();
            return *this;
        }

        self_type operator--(int junk) {
            m_enum.next(); // TODO
            return *this;
        }

        bool operator==(const self_type& rhs) {
            return m_enum.position() == rhs.m_enum.position();
        }

        bool operator!=(const self_type& other) const {return !(*this == other);}

        const reference operator*() const {
            m_cur_value = std::make_pair(m_enum.value().second,m_values[m_enum.position()]);
            return m_cur_value;
        }

        const pointer operator->() const {
            m_cur_value = std::make_pair(m_enum.value().second,m_values[m_enum.position()]);
            return &m_cur_value;
        }

        template<class K>
        self_type lower_bound(const K& key) {
            m_enum.next_geq(key);
            return *this;
        }
    };

    // Iterator routines.
    const_iterator begin() const { return const_iterator(m_values,m_ef,0,m_universe,m_values.size(),m_params); }
    const_iterator end() const { return const_iterator(m_values,m_ef,m_values.size(),m_universe,m_values.size(),m_params); }

    const_iterator lower_bound(const key_type &key) const {
        auto itr = begin();
        return itr.lower_bound(key);
    }

    void swap(ef_map &x) {
        m_values.swap(x.m_values);
        m_ef.swap(x.m_ef);
    }
    void verify() const {

    }

    // Size routines.
    size_type size() const { return m_values.size(); }
    bool empty() const { return m_values.size() == 0; }

    size_type bytes_used() const {
        return (m_ef.size()/8) + m_values.size()*sizeof(value_type);
    }

    friend bool operator==(const ef_map &x, const ef_map &y) {
        if (x.size() != y.size()) return false;
        return std::equal(x.begin(), x.end(), y.begin());
    }

    friend bool operator!=(const ef_map &x, const ef_map &y) {
        return !(x == y);
    }

    ef_map() {};

    template<class other_map_type>
    ef_map(other_map_type& other) {
        auto last = other.rbegin();
        m_universe = last->first.id() + 1;
        size_type n = other.size();

        {
            std::vector<size_type> points;
            std::transform(other.begin(),other.end(),std::back_inserter(points),[](const auto& pair){return pair.first.id();});

            succinct::bit_vector_builder bvb;
            quasi_succinct::compact_elias_fano::write(bvb, points.begin(),m_universe, n,m_params);
            succinct::bit_vector(&bvb).swap(m_ef);
        }

        std::transform(other.begin(),other.end(),std::back_inserter(m_values),[](const auto& pair){return pair.second;});
    }
};
