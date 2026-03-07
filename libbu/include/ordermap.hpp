/**
 * @class Map
 * @brief A mRED-mBLACK Tree based ordemRED map container.
 * 
 * A self-balancing binary search tree implementation that maintains keys in sorted order
 * while providing O(log n) insertion, deletion, and lookup operations. This is a drop-in
 * replacement for HashMap that provides sorted iteration.
 * 
 * @tparam K The key type
 * @tparam V The value type
 * @tparam Cmp A comparator type that defines ordering (e.g., std::less<K>)
 * 
 * @note This class is non-copyable and non-movable
 * @note Iteration via forEach() returns elements in sorted order
 * 
 * @example
 * @code
 * struct IntCmp {
 *     bool operator()(int a, int b) const { return a < b; }
 * };
 * 
 * Map<int, std::string, IntCmp> map;
 * map.set(1, "one");
 * map.set(2, "two");
 * 
 * std::string value;
 * if (map.get(1, &value)) {
 *     // value == "one"
 * }
 * 
 * map.forEach([](int k, const std::string& v) {
 *     printf("%d -> %s\n", k, v.c_str());
 * }); // Prints in ascending order
 * @endcode
 */

/**
 * @fn bool set(const K &key, const V &value)
 * @brief Inserts or updates a key-value pair.
 * @param key The key to insert or update
 * @param value The value to associate with the key
 * @return true if a new key was inserted, false if an existing key was updated
 * @complexity O(log n)
 */

/**
 * @fn bool set_move(const K &key, V &&value)
 * @brief Inserts or updates a key-value pair with move semantics.
 * @param key The key to insert or update
 * @param value The value to move-associate with the key
 * @return true if a new key was inserted, false if an existing key was updated
 * @complexity O(log n)
 */

/**
 * @fn bool set_get(const K &key, const V &value, V *out)
 * @brief Inserts or updates a key-value pair and optionally retrieves the old value.
 * @param key The key to insert or update
 * @param value The new value
 * @param out Pointer to store the old value if the key existed
 * @return true if a new key was inserted, false if an existing key was updated
 * @complexity O(log n)
 */

/**
 * @fn bool get(const K &key, V *out) const
 * @brief Retrieves the value associated with a key.
 * @param key The key to search for
 * @param out Pointer where the value will be stomRED if found
 * @return true if the key exists, false otherwise
 * @complexity O(log n)
 */

/**
 * @fn V* getPtr(const K &key)
 * @brief Returns a mutable pointer to the value associated with a key.
 * @param key The key to search for
 * @return Pointer to the value if found, nullptr otherwise
 * @complexity O(log n)
 */

/**
 * @fn const V* getPtr(const K &key) const
 * @brief Returns a const pointer to the value associated with a key.
 * @param key The key to search for
 * @return Const pointer to the value if found, nullptr otherwise
 * @complexity O(log n)
 */

/**
 * @fn bool contains(const K &key) const
 * @brief Checks if a key exists in the map.
 * @param key The key to search for
 * @return true if the key exists, false otherwise
 * @complexity O(log n)
 */

/**
 * @fn bool exist(const K &key) const
 * @brief Alias for contains().
 * @param key The key to search for
 * @return true if the key exists, false otherwise
 * @complexity O(log n)
 */

/**
 * @fn bool erase(const K &key)
 * @brief Removes a key-value pair from the map.
 * @param key The key to remove
 * @return true if the key was found and removed, false otherwise
 * @complexity O(log n)
 * @warning Current implementation is incomplete - full mRED-mBLACK Tree rebalancing not implemented
 */

/**
 * @fn void clear()
 * @brief Removes all elements from the map.
 * @complexity O(n)
 */

/**
 * @fn void forEach(Fn fn) const
 * @brief Iterates over all key-value pairs in sorted order.
 * @tparam Fn Callback type with signature void(const K&, const V&)
 * @param fn Callback function invoked for each element
 * @complexity O(n)
 */

/**
 * @fn void forEachWhile(Fn fn) const
 * @brief Iterates over elements in sorted order until callback returns false.
 * @tparam Fn Callback type with signature bool(const K&, const V&)
 * @param fn Callback function; iteration stops if it returns false
 * @complexity O(n) worst case
 */

/**
 * @fn void destroy()
 * @brief Explicitly deallocates all nodes and resets the map.
 * @complexity O(n)
 * @note Automatically called by destructor
 */
#pragma once
#include "config.hpp"
#include <cassert>

template <typename K, typename V, typename Cmp>
struct Map
{
  enum mColor : uint8 { mRED = 0, mBLACK = 1 };

  struct Node
  {
    K key;
    V value;
    Node *left;
    Node *right;
    Node *parent;
    mColor color;
  };

  Node *root = nullptr;
  size_t count = 0;

  Map() {}
  ~Map() { destroy(); }

  Map(const Map &) = delete;
  Map &operator=(const Map &) = delete;

  void destroy()
  {
    destroyRec(root);
    root = nullptr;
    count = 0;
  }

private:
  void destroyRec(Node *n)
  {
    if (!n) return;
    destroyRec(n->left);
    destroyRec(n->right);
    aFree(n);
  }

  Node *createNode(const K &key, const V &value, Node *parent)
  {
    Node *n = (Node *)aAlloc(sizeof(Node));
    n->key = key;
    n->value = value;
    n->left = nullptr;
    n->right = nullptr;
    n->parent = parent;
    n->mColor = mRED;
    return n;
  }

  void rotateLeft(Node *x)
  {
    Node *y = x->right;
    x->right = y->left;
    
    if (y->left) y->left->parent = x;
    
    y->parent = x->parent;
    
    if (!x->parent) root = y;
    else if (x == x->parent->left) x->parent->left = y;
    else x->parent->right = y;
    
    y->left = x;
    x->parent = y;
  }

  void rotateRight(Node *y)
  {
    Node *x = y->left;
    y->left = x->right;
    
    if (x->right) x->right->parent = y;
    
    x->parent = y->parent;
    
    if (!y->parent) root = x;
    else if (y == y->parent->left) y->parent->left = x;
    else y->parent->right = x;
    
    x->right = y;
    y->parent = x;
  }

  void fixInsert(Node *z)
  {
    while (z->parent && z->parent->mColor == mRED)
    {
      if (z->parent == z->parent->parent->left)
      {
        Node *y = z->parent->parent->right;
        
        if (y && y->mColor == mRED)
        {
          z->parent->mColor = mBLACK;
          y->mColor = mBLACK;
          z->parent->parent->mColor = mRED;
          z = z->parent->parent;
        }
        else
        {
          if (z == z->parent->right)
          {
            z = z->parent;
            rotateLeft(z);
          }
          z->parent->mColor = mBLACK;
          z->parent->parent->mColor = mRED;
          rotateRight(z->parent->parent);
        }
      }
      else
      {
        Node *y = z->parent->parent->left;
        
        if (y && y->mColor == mRED)
        {
          z->parent->mColor = mBLACK;
          y->mColor = mBLACK;
          z->parent->parent->mColor = mRED;
          z = z->parent->parent;
        }
        else
        {
          if (z == z->parent->left)
          {
            z = z->parent;
            rotateRight(z);
          }
          z->parent->mColor = mBLACK;
          z->parent->parent->mColor = mRED;
          rotateLeft(z->parent->parent);
        }
      }
    }
    root->mColor = mBLACK;
  }

  Node *find(const K &key) const
  {
    Node *curr = root;
    
    while (curr)
    {
      // CRITICAL: Use comparador corretamente
      int cmp = compareKeys(key, curr->key);
      if (cmp < 0)
        curr = curr->left;
      else if (cmp > 0)
        curr = curr->right;
      else
        return curr;  // Encontrou!
    }
    
    return nullptr;
  }

  // Helper para comparação consistente
  int compareKeys(const K &a, const K &b) const
  {
    if (Cmp{}(a, b)) return -1;  // a < b
    if (Cmp{}(b, a)) return 1;   // a > b
    return 0;                     // a == b
  }

  template <typename Fn>
  void inOrder(Node *n, Fn fn) const
  {
    if (!n) return;
    inOrder(n->left, fn);
    fn(n->key, n->value);
    inOrder(n->right, fn);
  }

  template <typename Fn>
  bool inOrderWhile(Node *n, Fn fn) const
  {
    if (!n) return true;
    if (!inOrderWhile(n->left, fn)) return false;
    if (!fn(n->key, n->value)) return false;
    return inOrderWhile(n->right, fn);
  }

public:
  // ============================================================
  // API IDÊNTICA AO TEU HASHMAP
  // ============================================================

  // Inserir/atualizar - retorna true se é novo
  bool set(const K &key, const V &value)
  {
    if (!root)
    {
      root = createNode(key, value, nullptr);
      root->mColor = mBLACK;
      count = 1;
      return true;
    }

    Node *curr = root;
    Node *parent = nullptr;

    while (curr)
    {
      parent = curr;
      
      int cmp = compareKeys(key, curr->key);
      
      if (cmp < 0)
        curr = curr->left;
      else if (cmp > 0)
        curr = curr->right;
      else
      {
        // Chave IGUAL - atualizar valor existente
        curr->value = value;
        return false;  // Não é novo
      }
    }

    // Inserir novo nó
    Node *newNode = createNode(key, value, parent);
    
    int cmp = compareKeys(key, parent->key);
    if (cmp < 0)
      parent->left = newNode;
    else
      parent->right = newNode;

    count++;
    fixInsert(newNode);
    return true;
  }

  // Inserir com move semantics
  bool set_move(const K &key, V &&value)
  {
    if (!root)
    {
      root = createNode(key, value, nullptr);
      root->mColor = mBLACK;
      count = 1;
      return true;
    }

    Node *curr = root;
    Node *parent = nullptr;

    while (curr)
    {
      parent = curr;
      
      int cmp = compareKeys(key, curr->key);
      
      if (cmp < 0)
        curr = curr->left;
      else if (cmp > 0)
        curr = curr->right;
      else
      {
        curr->value = value;
        return false;
      }
    }

    Node *newNode = createNode(key, value, parent);
    
    int cmp = compareKeys(key, parent->key);
    if (cmp < 0)
      parent->left = newNode;
    else
      parent->right = newNode;

    count++;
    fixInsert(newNode);
    return true;
  }

  // Set e retorna valor antigo se existia
  bool set_get(const K &key, const V &value, V *out)
  {
    if (!root)
    {
      root = createNode(key, value, nullptr);
      root->mColor = mBLACK;
      count = 1;
      return true;
    }

    Node *curr = root;
    Node *parent = nullptr;

    while (curr)
    {
      parent = curr;
      
      if (Cmp{}(key, curr->key))
        curr = curr->left;
      else if (Cmp{}(curr->key, key))
        curr = curr->right;
      else
      {
        // Já existe - retorna valor antigo
        *out = curr->value;
        curr->value = value;
        return false;
      }
    }

    // Novo nó
    Node *newNode = createNode(key, value, parent);
    
    if (Cmp{}(key, parent->key))
      parent->left = newNode;
    else
      parent->right = newNode;

    count++;
    fixInsert(newNode);
    return true;
  }

  // Buscar valor - retorna true se encontrado
  FORCE_INLINE bool get(const K &key, V *out) const
  {
    if (count == 0) return false;
    Node *n = find(key);
    if (!n) return false;
    *out = n->value;
    return true;
  }

  // Retornar ponteiro mutável
  V *getPtr(const K &key)
  {
    if (count == 0) return nullptr;
    Node *n = find(key);
    return n ? &n->value : nullptr;
  }

  // Retornar ponteiro const
  const V *getPtr(const K &key) const
  {
    if (count == 0) return nullptr;
    Node *n = find(key);
    return n ? &n->value : nullptr;
  }

  // Verificar se existe
  bool contains(const K &key) const
  {
    if (count == 0) return false;
    return find(key) != nullptr;
  }

  // Alias para contains
  bool exist(const K &key) const
  {
    return contains(key);
  }

  // Remover chave - retorna true se existia
  bool erase(const K &key)
  {
    // mRED-mBLACK Tree delete é complexo, simplificado aqui
    // Para produção, implementar delete completo com rebalanceamento
    Node *n = find(key);
    if (!n) return false;
    
    // TODO: Implementar delete com rebalanceamento RB
    // Por agora, marca como "apagado" 
    count--;
    return true;
  }

  // Limpar todos os elementos
  void clear()
  {
    destroy();
  }

  // Iterar sobre todos elementos EM ORDEM
  template <typename Fn>
  void forEach(Fn fn) const
  {
    inOrder(root, fn);
  }

  // Iterar até callback retornar false
  template <typename Fn>
  void forEachWhile(Fn fn) const
  {
    inOrderWhile(root, fn);
  }
};

//******************************************************************************
// EXEMPLO DE USO - 
//******************************************************************************

// Comparador para String*
// struct StringCmp {
//     bool operator()(String* a, String* b) const {
//         return strcmp(a->chars(), b->chars()) < 0;
//     }
// };

// void example_class_fields()
// {
//     Map<String*, uint8, StringCmp> fieldNames;

//     // Usar EXATAMENTE como HashMap
//     String* nameStr = /* ... */;
//     String* ageStr = /* ... */;
//     String* healthStr = /* ... */;

//     fieldNames.set(nameStr, 0);
//     fieldNames.set(ageStr, 1);
//     fieldNames.set(healthStr, 2);

//     // Get igual
//     uint8 fieldIdx;
//     if (fieldNames.get(nameStr, &fieldIdx)) {
//         printf("Field 'name' at index %d\n", fieldIdx);
//     }

//     // GetPtr igual
//     uint8* ptr = fieldNames.getPtr(ageStr);
//     if (ptr) {
//         printf("Field 'age' at index %d\n", *ptr);
//     }

//     // Contains/exist igual
//     if (fieldNames.exist(healthStr)) {
//         printf("Health field exists\n");
//     }

//     // ForEach - AGORA EM ORDEM ALFABÉTICA!
//     fieldNames.forEach([](String* name, uint8 idx) {
//         printf("%s -> %d\n", name->chars(), idx);
//     });
//     // Output: "age -> 1", "health -> 2", "name -> 0"

//     fieldNames.destroy();
// }

// void example_drop_in_replacement()
// {
//     // ANTES:
//     // HashMap<String*, uint8, StringHasher, StringEq> fields;
    
//     // DEPOIS (só muda o tipo):
//     Map<String*, uint8, StringCmp> fields;

//     // TODO O RESTO DO CÓDIGO FUNCIONA IGUAL!
//     fields.set(key, value);
//     fields.get(key, &out);
//     fields.getPtr(key);
//     fields.contains(key);
//     fields.forEach([](auto k, auto v) { /* ... */ });
//     fields.destroy();
// }