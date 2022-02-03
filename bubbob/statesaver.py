"""
A pure Python implementation of statesaver.c that runs on top of PyPy.
See description in statesaver.c.
Difference: this supports new-style instances too.
Use statesaver.standard_build() as the inst_build() if you want.
"""

from _pickle_support import generator_new
import types

def standard_build(self):
    if type(self) is types.InstanceType:
        # old-style instance
        return types.InstanceType(self.__class__)
    else:
        # new-style instance
        return type(self).__new__(type(self))

# ____________________________________________________________

def not_copied(x, memo):
    return x

def copy_custom_instance(x, memo):
    try:
        return memo[id(x)]
    except KeyError:
        y = x.inst_build()
        memo[id(x)] = y
        for key, value in list(x.__dict__.items()):
            y.__dict__[key] = copyrec(value, memo)
        return y

def copy_tuple(x, memo):
    return tuple([copyrec(item, memo) for item in x])

def copy_list(x, memo):
    try:
        return memo[id(x)]
    except KeyError:
        y = []
        memo[id(x)] = y
        for item in x:
            y.append(copyrec(item, memo))
        return y

def copy_dict(x, memo):
    try:
        return memo[id(x)]
    except KeyError:
        y = {}
        memo[id(x)] = y
        for key, value in list(x.items()):
            y[copyrec(key, memo)] = copyrec(value, memo)
        return y

def copy_function(x, memo):
    if not x.__defaults__:
        return x     # not copied
    try:
        return memo[id(x)]
    except KeyError:
        y = types.FunctionType(x.__code__, x.__globals__, x.__name__)
        memo[id(x)] = y
        y.__defaults__ = copyrec(x.__defaults__, memo)
        return y

def copy_method(x, memo):
    return types.MethodType(copyrec(x.__func__, memo),
                            copyrec(x.__self__, memo),
                            x.__self__.__class__)

def copy_generator(x, memo):
    try:
        return memo[id(x)]
    except KeyError:
        y = generator_new(copyrec(x.gi_frame, memo), x.gi_running)
        memo[id(x)] = y
        return y

def copy_frame(x, memo):
    try:
        return memo[id(x)]
    except KeyError:
        frame_new, args, state = x.__reduce__()
        y = frame_new(*args)
        memo[id(x)] = y
        newstate = []
        for item in state:
            if not (item is x.f_globals or item is x.f_builtins):
                item = copyrec(item, memo)
            newstate.append(item)
        y.__setstate__(newstate)
        return y

def copy_seqiter(x, memo):
    try:
        return memo[id(x)]
    except KeyError:
        # XXX self-recursion is not correctly handled here
        seqiter_new, args = x.__reduce__()
        args = [copyrec(item, memo) for item in args]
        y = seqiter_new(*args)
        memo[id(x)] = y
        return y

# ____________________________________________________________

type_handlers = {tuple: copy_tuple,
                 list: copy_list,
                 dict: copy_dict,
                 types.FunctionType: copy_function,
                 types.MethodType: copy_method,
                 types.GeneratorType: copy_generator,
                 types.FrameType: copy_frame,
                 type(iter([])): copy_seqiter,
                 }

def no_handler_found(x, memo):
    if hasattr(x, '__dict__') and hasattr(x.__class__, 'inst_build'):
        handler = copy_custom_instance
    else:
        handler = not_copied
    type_handlers[x.__class__] = handler
    return handler(x, memo)

def copyrec(x, memo):
    try:
        cls = x.__class__
    except AttributeError:
        return x      # 'cls' is likely an old-style class object
    return type_handlers.get(cls, no_handler_found)(x, memo)

def copy(x):
    return copyrec(x, {})
