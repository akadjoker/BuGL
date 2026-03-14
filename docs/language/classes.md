# Classes

## Basic Class

```bulang
class Animal {
    def init(name, sound) {
        self.name  = name;
        self.sound = sound;
    }

    def speak() {
        write("{} says: {}\n", self.name, self.sound);
    }
}

var cat = Animal("Cat", "Meow");
cat.speak();   // Cat says: Meow
```

## Inheritance

```bulang
class Dog extends Animal {
    def init(name) {
        super.init(name, "Woof!");
    }

    def fetch(item) {
        write("{} fetches the {}\n", self.name, item);
    }
}

var dog = Dog("Rex");
dog.speak();          // Rex says: Woof!
dog.fetch("ball");    // Rex fetches the ball
```

## Fields with Defaults

```bulang
class Point {
    var x = 0;
    var y = 0;

    def init(px, py) {
        self.x = px;
        self.y = py;
    }

    def distanceTo(other) {
        var dx = self.x - other.x;
        var dy = self.y - other.y;
        return sqrt(dx*dx + dy*dy);
    }
}
```

## Method Chaining

```bulang
class Builder {
    var value = 0;

    def init(v)    { self.value = v; }
    def add(x)     { self.value += x; return self; }
    def multiply(x){ self.value *= x; return self; }
    def get()      { return self.value; }
}

var b = Builder(10);
b.add(5).multiply(2).get();   // 30
```

## Arrays of Classes

```bulang
var enemies = [];
enemies.push(Dog("Buddy"));
enemies.push(Dog("Max"));

foreach (var e in enemies) {
    e.speak();
}
```

## Closures inside Methods

```bulang
class Counter {
    var count = 0;

    def makeStepper(step) {
        def stepper() {
            self.count += step;
            return self.count;
        }
        return stepper;
    }
}

var c   = Counter();
var by2 = c.makeStepper(2);
by2();   // 2
by2();   // 4
```
