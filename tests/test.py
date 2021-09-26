string = "Python"
number = 121
double = 1.5
boolean = False

table = {
    "a": "AA",
    "b": "BB",
    "c": "CC",
    "d": "DD",
    10: 10
}

pyList = [
    "List",
    12
]

pyTuple = (
    "Tuple",
    12
)

pySet = {
    "Set",
    12
}


def callme(*args):
    # print("From CallMe", flush=True, end=" ")
    # print(args, flush=True)
    return None


def get_called(func):
    # if func == None:
    #     print("got none")
    # else:
    #     func()
    # print("got: ", func)
    try:
        # print(func(55))
        func(55)
    except Exception as a:
        # print("Lua function errored out")
        pass

    # func(1)

def handle_iter(co):
    it = iter(co)
    # print(f"{it = }")
    a = next(co)
    # print(f"{a = }")

def handle_iter_all(co):
    for i in co:
        # print(i)
        i


def c_range(x):
    a = 0
    if x < 1:
        raise Exception("x should greater than 0")
    while a < x:
        yield a
        a += 1
