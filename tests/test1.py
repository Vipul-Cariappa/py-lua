class Circle:
    def __init__(self, radius):
        self.radius = radius

    def get_area(self):
        self.area = self.radius * self.radius * 3.14
        return self.area

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

class Call:
    def __init__(self, *args, **kwargs):
        pass

    def __call__(self, *args, **kwargs):
        return "Call.__call__ function"

def handle_add(rect1, rect2):
    return rect1 + rect2

def handle_add_int(rect):
    return rect + 10

def handle_sub(rect1, rect2):
    return rect1 - rect2

def handle_sub_int(rect):
    return rect - 10

def handle_mul(rect):
    return rect * 2

def handle_div(rect):
    return rect / 2

def handle_call(call_obj):
    return call_obj()


def pass_object(obj):
    assert obj.length == 1
    assert obj.breadth == 2
    

def operations_on_objects(obj1, obj2):
    # arithmetic operations
    assert (obj1 + obj2).length == 11
    assert (obj1 - obj2).breadth == -18
    assert (obj1 * 5).breadth == 10
    assert (obj2 / 2).length == 5

    # relational operators
    assert (obj1 < obj2) == True
    assert (obj1 <= obj2) == True
    assert (obj1 == obj2) == False
    assert (obj1 != obj2) == True
    assert (obj1 > obj2) == False
    assert (obj1 >= obj2) == False
