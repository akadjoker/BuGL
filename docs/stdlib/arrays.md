# Arrays

## Creation

```bulang
var empty = [];
var nums  = [1, 2, 3, 4, 5];
var mixed = [1, "hello", true, nil];
var nested = [[1,2], [3,4], [5,6]];
```

## Access

```bulang
nums[0];         // 1  (zero-indexed)
nums[len(nums)-1];  // last element
nested[1][0];    // 3
```

## Methods

| Method | Description |
|--------|-------------|
| `.push(val)` | Add to end |
| `.pop()` | Remove and return last |
| `.back()` | Return last without removing |
| `.first()` | Return first element |
| `.last()` | Return last element |
| `.insert(i, val)` | Insert at index |
| `.remove(i)` | Remove at index |
| `.find(val)` | Index of value (-1 if not found) |
| `.contains(val)` | True if value exists |
| `.count(val)` | Count occurrences |
| `.reverse()` | Reverse in place |
| `.slice(start, end)` | Return sub-array |
| `.concat(arr)` | Concatenate with another array |
| `.join(sep)` | Join to string |
| `.fill(val)` | Fill with value |
| `.clear()` | Remove all elements |
| `.length()` | Number of elements |

## Global `len()`

```bulang
len(arr);   // same as arr.length()
```

## Examples

```bulang
var a = [3, 1, 4, 1, 5, 9];
a.push(2).push(6);
print(len(a));          // 8
print(a.contains(9));   // true
print(a.find(5));       // 4

var b = a.slice(2, 5);  // [4, 1, 5]
var c = a.concat([7, 8]); 
print(a.join(", "));     // "3, 1, 4, 1, 5, 9, 2, 6"
```

## foreach over Array

```bulang
var sum = 0;
foreach (var v in nums) {
    sum += v;
}
```
