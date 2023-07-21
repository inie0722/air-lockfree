#include <atomic>
#include <cstddef>
#include <stdexcept>

namespace air
{
    template <typename T, template <typename Elem> class Atomic = std::atomic>
    class circular_buffer final
    {
    public:
        using value_type = T;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using reference = value_type &;
        using const_reference = const value_type &;
        using pointer = value_type *;
        using const_pointer = const value_type *;

    private:
        struct Node
        {
            Atomic<size_type> write_count;
            value_type value;
        };

        Atomic<size_type> size_;
        size_type capacity_;
        Node data_[];

    public:
        circular_buffer(size_type capacity)
        {
            this->capacity_ = capacity;
            this->size_ = 0;
            for (size_type i = 0; i < capacity; i++)
            {
                this->data_[i].write_count = 0;
            }
        }

        template <bool is_notify>
        void push(const value_type &val)
        {
            auto index = size_.fetch_add(1) % capacity_;
            data_[index].value = val;
            data_[index].write_count.fetch_add(1);
            if constexpr (is_notify == true)
                data_[index].write_count.notify_all();
        }

        std::pair<size_type, pointer> alloc()
        {
            auto index = size_.fetch_add(1) % capacity_;
            return {index, &data_[index].value};
        }

        template <bool is_notify>
        void commit(size_type index)
        {
            data_[index].write_count.fetch_add(1);
            if constexpr (is_notify == true)
                data_[index].write_count.notify_all();
        }

        value_type at(size_type index) const
        {
            size_type i = index % capacity_;
            size_type write_count = data_[i].write_count;
            if (write_count != (index / capacity_) + 1)
                throw std::runtime_error("data has been overwritten");
            value_type val = data_[i].value;
            if (write_count != data_[i].write_count)
                throw std::runtime_error("data has been overwritten");

            return val;
        }

        value_type &operator[](size_type index)
        {
            auto i = index % capacity_;
            return data_[i].value;
        }

        const value_type &operator[](size_type index) const
        {
            auto i = index % capacity_;
            return data_[i].value;
        }

        void wait(size_type index) const
        {
            auto i = index % capacity_;
            data_[i].write_count.wait((index / capacity_) + 1);
        }

        size_type size() const
        {
            return size_;
        }

        size_type capacity() const
        {
            return capacity_;
        }

        static size_type get_memory_size(size_type max_size)
        {
            return sizeof(circular_buffer) + (sizeof(Node) * max_size);
        }
    };
}
