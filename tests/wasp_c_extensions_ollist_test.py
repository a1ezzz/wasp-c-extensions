
import pytest

from wasp_c_extensions.ollist import WOrderedLinkedList


class TestWOrderedLinkedList:

    def test(self):
        l = WOrderedLinkedList()

        object1 = object()
        object2 = object()
        object3 = object()

        l.push(3, object1)
        l.push(2, object2)
        l.push(1, object3)

        l.next()
        l.pull()

        # assert(l.next() is object3)
        # assert(l.pull() is object3)
        # assert(l.pull() is object2)
        # assert(l.next() is object1)
        # assert(l.pull() is object1)
