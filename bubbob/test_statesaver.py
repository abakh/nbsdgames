import py
import statesaver
import new

def test_list():
    lst = [None, 12, "hello", 3.4, ("foo", (), [])]
    lst1 = statesaver.copy(lst)
    assert lst1 == lst
    assert lst1 is not lst
    assert lst1[-1][-1] is not lst[-1][-1]

def test_dict():
    dct = {1: "hi", 2: {}}
    dct1 = statesaver.copy(dct)
    assert dct1 == dct
    assert dct1 is not dct
    assert dct1[2] is not dct[2]

def test_instance():
    class Foo:
        def inst_build(self):
            return Bar()
    class Bar:
        pass
    x = Foo()
    x.attr = [1, 2, 3]
    y = statesaver.copy(x)
    assert y.__class__ is Bar
    assert y.attr == [1, 2, 3]
    assert y.attr is not x.attr

glob = 2
def test_function():
    # XXX closures not supported
    def func(x, y=[]):
        assert glob == 2
        y.append(x)
        return y
    l = func(5)
    l = func(6)
    assert l == [5, 6]
    func1 = statesaver.copy(func)
    l = func(7)
    l = func(8)
    assert l == [5, 6, 7, 8]
    l = func1(9)
    l = func1(10)
    assert l == [5, 6, 9, 10]

def test_method():
    def func(x, y=[]):
        assert glob == 2
        y.append(x)
        return y
    m = new.instancemethod(func, {})
    assert m() == [{}]
    m1 = statesaver.copy(m)
    assert m() == [{}, {}]
    assert m() == [{}, {}, {}]
    assert m1() == [{}, {}]
    assert m1() == [{}, {}, {}]
    l = m1()
    assert l[0] is l[1] is l[2] is l[3]

def test_generator():
    def gfunc():
        lst = [5, 6]
        yield lst.pop()
        yield lst.pop()
    g = gfunc()
    assert next(g) == 6
    g1 = statesaver.copy(g)
    assert next(g) == 5
    py.test.raises(StopIteration, g.__next__)
    assert next(g1) == 5
    py.test.raises(StopIteration, g1.__next__)

def test_exhausted_gen():
    def gfunc():
        yield 5
    g = gfunc()
    for i in g:
        print(i)
    g1 = statesaver.copy(g)
    assert iter(g1) is g1
    py.test.raises(StopIteration, g1.__next__)
    g2 = statesaver.copy(g1)
    assert iter(g2) is g2
    py.test.raises(StopIteration, g2.__next__)

def test_seqiter():
    from collections import UserList
    seq = UserList([2, 4, 6, 8])
    it = iter(seq)
    assert next(it) == 2
    assert next(it) == 4
    it1 = statesaver.copy(it)
    assert list(it) == [6, 8]
    assert list(it1) == [6, 8]

def test_tupleiter():
    tup = (2, 4, 6, 8)
    it = iter(tup)
    assert next(it) == 2
    assert next(it) == 4
    it1 = statesaver.copy(it)
    assert list(it) == [6, 8]
    assert list(it1) == [6, 8]

def test_listiter():
    lst = [2, 4, 6, 8]
    it = iter(lst)
    assert next(it) == 2
    assert next(it) == 4
    it1 = statesaver.copy(it)
    lst.append(10)
    assert list(it) == [6, 8, 10]
    assert list(it1) == [6, 8]

def test_stringiter():
    s = "hello"
    it = iter(s)
    assert next(it) == 'h'
    assert next(it) == 'e'
    it1 = statesaver.copy(it)
    assert list(it) == ['l', 'l', 'o']
    assert list(it1) == ['l', 'l', 'o']
