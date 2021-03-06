class Circle:
    def __init__(self, radius):
        self.radius = radius

    def get_area(self):
        self.area = self.radius * self.radius * 3.14
        return self.area

    def other(self, a, b):
        assert a == 1
        assert b == 2

    def __add__(self, other):
        if isinstance(other, Circle):
            return Circle(self.radius + other.radius)
        return Circle(self.radius + other)
    
    def __sub__(self, other):
        if isinstance(other, Circle):
            return Circle(self.radius - other.radius)
        return Circle(self.radius - other)
    
    def __mul__(self, other):
        return Circle(self.radius * other)
    
    def __div__(self, other):
        return Circle(self.radius / other)

    def __eq__(self, other):
        if self.radius == other.radius:
            return True
        return False
    
    def __ne__(self, other):
        if self.radius != other.radius:
            return True
        return False

    def __ge__(self, other):
        if self.radius >= other.radius:
            return True
        return False
    
    def __le__(self, other):
        if self.radius <= other.radius:
            return True
        return False
    
    def __gt__(self, other):
        if self.radius > other.radius:
            return True
        return False
    
    def __lt__(self, other):
        if self.radius < other.radius:
            return True
        return False

    def __str__(self):
        return f"Circle(radius={self.radius})"


class Call:
    def __init__(self, *args, **kwargs):
        pass

    def __call__(self, *args, **kwargs):
        return "Call.__call__ function"

c1 = Circle(1)
c2 = Circle(2)
f = Call()

def handle_call(call_obj):
    return call_obj()


def pass_object(obj):
    assert str(obj) == "Rectangle(length = 1, Breadth = 2)"
    assert obj.length == 1
    assert obj.breadth == 2
    

def operations_on_objects(obj1, obj2):
    # arithmetic operations
    assert (obj1 + obj2).length == 11
    assert (obj1 - obj2).breadth == -18
    assert (obj1 * 5).breadth == 10
    assert (obj2 / 2).length == 5

    # unary operations
    assert (-obj1).length == -1
    assert (-obj1).breadth == -2

    # relational operators
    assert (obj1 < obj2) == True
    assert (obj1 <= obj2) == True
    assert (obj1 == obj2) == False
    assert (obj1 != obj2) == True
    assert (obj1 > obj2) == False
    assert (obj1 >= obj2) == False
    assert len(obj1) == 0
    assert ~obj1 == -1

def insert_new(obj):
    obj.python = "New Object"

def test_lua_class(rect):
    r = rect(4, 6)
    r.other(2, 3, False)
    assert r.get_area() == 24
    assert r.length == 4
    assert r.breadth == 6

def return_me(o):
    return o
