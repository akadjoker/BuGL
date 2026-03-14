# Buffers

Binary buffers for low-level data manipulation and GPU uploads.

## Creation

```bulang
var buf = Buffer(256);               // 256 bytes, default UINT8
var f32 = Buffer(64, BUFFER_FLOAT32); // float32 buffer
```

## Write / Read

```bulang
buf.writeInt(1234);
buf.writeFloat(3.14);
buf.writeString("BuGL");

buf.rewind();   // go back to start

var i = buf.readInt();     // 1234
var f = buf.readFloat();   // 3.14
var s = buf.readString(4); // "BuGL"
```

## Navigation

| Method | Description |
|--------|-------------|
| `.rewind()` | Seek to position 0 |
| `.seek(pos)` | Seek to position |
| `.tell()` | Current position |
| `.skip(n)` | Advance N bytes |
| `.remaining()` | Bytes left |

## All Read/Write Methods

| Method | Type |
|--------|------|
| `.writeByte(b)` / `.readByte()` | Uint8 |
| `.writeShort(s)` / `.readShort()` | Int16 |
| `.writeUShort(s)` / `.readUShort()` | Uint16 |
| `.writeInt(i)` / `.readInt()` | Int32 |
| `.writeUInt(i)` / `.readUInt()` | Uint32 |
| `.writeFloat(f)` / `.readFloat()` | Float32 |
| `.writeDouble(d)` / `.readDouble()` | Float64 |
| `.writeString(s)` / `.readString(n)` | String |

## Utility

```bulang
buf.fill(0);              // zero all bytes
buf.save("data.bin");     // save to file
free(buf);                // manual release
```

## OpenGL Usage

```bulang
var verts = Buffer(12 * 4, BUFFER_FLOAT32);
verts.writeFloat(-0.5); verts.writeFloat(-0.5); verts.writeFloat(0.0);
verts.writeFloat( 0.5); verts.writeFloat(-0.5); verts.writeFloat(0.0);
verts.writeFloat( 0.0); verts.writeFloat( 0.5); verts.writeFloat(0.0);

glBufferData(GL_ARRAY_BUFFER, verts, GL_STATIC_DRAW);
```
