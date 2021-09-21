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
    print("got: ", func)
    try:
        print(func(55))
    except Exception as a:
        print("Lua function errored out")

    # func(1)
