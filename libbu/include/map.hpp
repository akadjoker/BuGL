/**
 * @brief Open-addressed hash map with linear probing and tombstone deletion.
 *
 * A generic hash map implementation using open addressing with linear probing.
 * Supports efficient insertion, deletion, and lookup with O(1) average time complexity.
 *
 * @tparam K The key type. Must be hashable and comparable.
 * @tparam V The value type.
 * @tparam Hasher A callable type that computes hash values for keys of type K.
 *                 Must implement: size_t operator()(const K&) const
 * @tparam Eq A callable type for key equality comparison.
 *            Must implement: bool operator()(const K&, const K&) const
 *
 * @details
 * - Uses open addressing with linear probing for collision resolution
 * - Tombstone entries mark deleted slots to maintain probe sequences
 * - Capacity is always a power of 2 for efficient modulo operations via bitmask
 * - Automatically grows when load factor exceeds MAX_LOAD (0.75)
 * - All operations assume K and V are POD types (Plain Old Data)
 *
 * @note This class is non-copyable. Manual destroy() must be called for cleanup.
 *
 * @example
 * ```cpp
 * struct MyHasher {
 *     size_t operator()(int k) const { return static_cast<size_t>(k); }
 * };
 * struct MyEq {
 *     bool operator()(int a, int b) const { return a == b; }
 * };
 *
 * HashMap<int, float, MyHasher, MyEq> map;
 * map.set(1, 3.14f);
 * float value;
 * if (map.get(1, &value)) {
 *     // Found: value = 3.14f
 * }
 * map.destroy();
 * ```
 */
#pragma once
#include "config.hpp"
#include <cassert>
#include <cstring>

template <typename K, typename V, typename Hasher, typename Eq>
struct HashMap
{
  enum State : uint8
  {
    EMPTY = 0, // Explícito! memset garante isso
    FILLED = 1,
    TOMBSTONE = 2
  };

  struct Entry
  {
    K key;
    V value;
    size_t hash;
    State state;
  };

  Entry *entries = nullptr;
  size_t capacity = 0;
  size_t count = 0;
  size_t tombstones = 0;

  static constexpr float MAX_LOAD = 0.75f;

  HashMap() {}

  ~HashMap() { destroy(); }

  HashMap(const HashMap &) = delete;
  HashMap &operator=(const HashMap &) = delete;

  void destroy()
  {
    if (!entries)
      return;
    aFree(entries);
    entries = nullptr;
    capacity = count = tombstones = 0;
  }

  size_t mask() const { return capacity - 1; }

  Entry *findSlot(const K &key, size_t hash)
  {
    size_t index = hash & mask();
    Entry *tomb = nullptr;

    for (;;)
    {
      Entry *e = &entries[index];

      if (e->state == EMPTY)
      {
        return tomb ? tomb : e;
      }

      if (e->state == TOMBSTONE)
      {
        if (!tomb)
          tomb = e;
      }
      else if (e->hash == hash && Eq{}(e->key, key))
      {
        return e;
      }

      index = (index + 1) & mask();
    }
  }

  Entry *findFilled(const K &key, size_t hash) const
  {
    if (capacity == 0)
      return nullptr;
    size_t index = hash & mask();

    for (;;)
    {
      Entry *e = &entries[index];
      if (e->state == EMPTY)
        return nullptr;
      if (e->state == FILLED && e->hash == hash && Eq{}(e->key, key))
        return e;
      index = (index + 1) & mask();
    }
  }

  void adjustCapacity(size_t newCap)
  {
    // Garantir power-of-two para mask() funcionar
    assert((newCap & (newCap - 1)) == 0 && "Capacity must be power of 2");

    Entry *old = entries;
    size_t oldCap = capacity;

    entries = (Entry *)aAlloc(newCap * sizeof(Entry));
    std::memset(entries, 0, newCap * sizeof(Entry)); // State = EMPTY = 0

    capacity = newCap;
    count = 0;
    tombstones = 0;

    if (old)
    {
      for (size_t i = 0; i < oldCap; i++)
      {
        Entry *e = &old[i];
        if (e->state == FILLED)
        {
          Entry *dst = findSlot(e->key, e->hash);
          std::memcpy(dst, e, sizeof(Entry)); // POD copy, mais rápido
          count++;
        }
      }
      aFree(old);
    }
  }

  void maybeGrow()
  {
    if (capacity == 0 || (count + tombstones + 1) > capacity * MAX_LOAD)
    {
      size_t newCap = capacity == 0 ? 16 : capacity * 2;
      adjustCapacity(newCap);
    }
  }

  bool set(const K &key, const V &value)
  {
    maybeGrow();
    size_t h = Hasher{}(key);
    Entry *e = findSlot(key, h);

    bool isNew = (e->state != FILLED);
    if (isNew)
    {
      if (e->state == TOMBSTONE)
        tombstones--;
      e->key = key;
      e->hash = h;
      e->state = FILLED;
      count++;
    }
    e->value = value;
    return isNew;
  }

  bool set_move(const K &key, V &&value)
  {
    maybeGrow();
    size_t h = Hasher{}(key);
    Entry *e = findSlot(key, h);

    bool isNew = (e->state != FILLED);
    if (isNew)
    {
      if (e->state == TOMBSTONE)
        tombstones--;
      e->key = key;
      e->hash = h;
      e->state = FILLED;
      count++;
    }
    e->value = value;
    return isNew;
  }

  bool set_get(const K &key, const V &value, V *out)
  {
    maybeGrow();
    size_t h = Hasher{}(key);
    Entry *e = findSlot(key, h);

    bool isNew = (e->state != FILLED);
    if (isNew)
    {
      if (e->state == TOMBSTONE)
        tombstones--;
      e->key = key;
      e->hash = h;
      e->state = FILLED;
      count++;
    }
    else
    {
      *out = e->value;
    }
    e->value = value;
    return isNew;
  }

  FORCE_INLINE bool get(const K &key, V *out) const
  {
    if (count == 0)
      return false;
    size_t h = Hasher{}(key);
    Entry *e = findFilled(key, h);
    if (!e)
      return false;
    *out = e->value;
    return true;
  }

  V *getPtr(const K &key)
  {
    if (count == 0)
      return nullptr;
    size_t h = Hasher{}(key);
    Entry *e = findFilled(key, h);
    if (!e)
      return nullptr;
    return &e->value;
  }

  const V *getPtr(const K &key) const
  {
    if (count == 0)
      return nullptr;
    size_t h = Hasher{}(key);
    Entry *e = findFilled(key, h);
    if (!e)
      return nullptr;
    return &e->value;
  }

  bool contains(const K &key) const
  {
    if (count == 0)
      return false;
    size_t h = Hasher{}(key);
    return findFilled(key, h) != nullptr;
  }

  bool erase(const K &key)
  {
    if (count == 0)
      return false;
    size_t h = Hasher{}(key);
    Entry *e = findFilled(key, h);
    if (!e)
      return false;

    e->state = TOMBSTONE;
    count--;
    tombstones++;
    return true;
  }

  void clear()
  {
    if (entries)
    {
      std::memset(entries, 0, capacity * sizeof(Entry));
      count = 0;
      tombstones = 0;
    }
  }
  bool exist(const K &key) const
  {
    if (count == 0)
      return false;
    size_t h = Hasher{}(key);
    return findFilled(key, h) != nullptr;
  }

  template <typename Fn>
  void forEach(Fn fn) const
  {
    for (size_t i = 0; i < capacity; i++)
    {
      if (entries[i].state == FILLED)
      {
        fn(entries[i].key, entries[i].value);
      }
    }
  }

  template <typename Fn>
  void forEachWhile(Fn fn) const
  {
    for (size_t i = 0; i < capacity; i++)
    {
      if (entries[i].state == FILLED)
      {
        if (!fn(entries[i].key, entries[i].value))
          break;
      }
    }
  }
};

//******************************************************************************
// Hasher/Eq para int

// struct IntHasher {
//     size_t operator()(int k) const { return static_cast<size_t>(k); }
// };
// struct IntEq {
//     bool operator()(int a, int b) const { return a == b; }
// };

// void example_basic()
// {
//     HashMap<int, float, IntHasher, IntEq> scores;

//     // Set valores
//     scores.set(1, 100.0f);
//     scores.set(2, 95.5f);
//     scores.set(3, 87.3f);

//     // Get valor
//     float score;
//     if (scores.get(2, &score)) {
//         printf("Player 2 score: %.1f\n", score);
//     }

//     // Get pointer (sem cópia)
//     float* ptr = scores.getPtr(1);
//     if (ptr) {
//         *ptr += 10.0f; // Modifica direto
//     }

//     // Iterar
//     scores.forEach([](int id, float score) {
//         printf("Player %d: %.1f\n", id, score);
//     });

//     scores.destroy();
// }

// void example_entity_components()
// {
//     struct Transform {
//         float x, y, z;
//         float rotX, rotY, rotZ;
//     };

//     struct EntityIDHasher {
//         size_t operator()(uint64 id) const { return static_cast<size_t>(id); }
//     };
//     struct EntityIDEq {
//         bool operator()(uint64 a, uint64 b) const { return a == b; }
//     };

//     HashMap<uint64, Transform, EntityIDHasher, EntityIDEq> transforms;

//     // Adicionar componentes
//     transforms.set(1001, {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f});
//     transforms.set(1002, {10.0f, 5.0f, 3.0f, 0.0f, 90.0f, 0.0f});

//     // Modificar direto via pointer
//     Transform* t = transforms.getPtr(1001);
//     if (t) {
//         t->x += 1.0f;
//         t->y += 0.5f;
//     }

//     // set_get - retorna valor antigo
//     Transform oldTransform;
//     bool existed = !transforms.set_get(1001, {5.0f, 5.0f, 5.0f, 0, 0, 0}, &oldTransform);
//     if (existed) {
//         printf("Old position: %.1f, %.1f, %.1f\n", oldTransform.x, oldTransform.y, oldTransform.z);
//     }

//     transforms.destroy();
// }

// void example_string_to_int()
// {
//     struct StrHasher {
//         size_t operator()(const char* str) const {
//             uint64 hash = 0xcbf29ce484222325ULL;
//             while (*str) {
//                 hash ^= static_cast<uint8>(*str++);
//                 hash *= 0x100000001b3ULL;
//             }
//             return static_cast<size_t>(hash);
//         }
//     };
//     struct StrEq {
//         bool operator()(const char* a, const char* b) const {
//             return strcmp(a, b) == 0;
//         }
//     };

//     HashMap<const char*, int, StrHasher, StrEq> nameToID;

//     nameToID.set("player", 1);
//     nameToID.set("enemy", 2);
//     nameToID.set("boss", 3);

//     int id;
//     if (nameToID.get("boss", &id)) {
//         printf("Boss ID: %d\n", id);
//     }

//     // Remover
//     nameToID.erase("enemy");

//     nameToID.destroy();
// }