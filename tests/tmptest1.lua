require("pylua")

py = Python.PyLoad("test")

Python.PyUnLoad(py)

-- print(py.number)

Python.PyUnLoad(py)
