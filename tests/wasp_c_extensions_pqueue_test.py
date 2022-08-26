
import pytest

from wasp_c_extensions.pqueue import WPriorityQueue


class TestWOrderedLinkedList:

    def test(self):
        q = WPriorityQueue()

        object1 = object()
        object2 = object()
        object3 = object()

        q.push(3, object1)
        q.push(2, object2)
        q.push(1, object3)

        q.next()
        q.pull()

        # assert(q.next() is object3)
        # assert(q.pull() is object3)
        # assert(q.pull() is object2)
        # assert(q.next() is object1)
        # assert(q.pull() is object1)
