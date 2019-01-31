
import pytest

from wasp_c_extensions.queue import WMCQueue


class TestWMCQueue:

	def test(self):
		q = WMCQueue()
		assert(q.count() == 0)
		assert(q.has(0) is False)
		assert(q.has(1) is False)
		assert(q.has(2) is False)
		assert(q.has(3) is False)
		assert(q.has(4) is False)

		q.push(None)
		assert(q.count() == 0)
		assert(q.has(0) is False)
		assert(q.has(1) is False)
		assert(q.has(2) is False)
		assert(q.has(3) is False)
		assert(q.has(4) is False)

		msg_index1 = q.subscribe()
		assert(msg_index1 == 0)
		assert(q.count() == 0)
		assert(q.has(0) is False)
		assert(q.has(1) is False)
		assert(q.has(2) is False)
		assert(q.has(3) is False)
		assert(q.has(4) is False)

		q.push(None)
		assert(q.count() == 1)
		assert(q.has(0) is True)
		assert(q.has(1) is False)
		assert(q.has(2) is False)
		assert(q.has(3) is False)
		assert(q.has(4) is False)

		pytest.raises(KeyError, q.pop, 1)
		assert(q.pop(msg_index1) is None)
		assert(q.count() == 0)
		assert(q.has(0) is False)
		assert(q.has(1) is False)
		assert(q.has(2) is False)
		assert(q.has(3) is False)
		assert(q.has(4) is False)
		msg_index1 += 1

		msg_index2 = q.subscribe()
		assert(msg_index2 == 1)
		assert(q.count() == 0)
		assert(q.has(0) is False)
		assert(q.has(1) is False)
		assert(q.has(2) is False)
		assert(q.has(3) is False)
		assert(q.has(4) is False)

		q.push(1)
		q.push(2)
		assert(q.count() == 2)
		assert(q.has(0) is False)
		assert(q.has(1) is True)
		assert(q.has(2) is True)
		assert(q.has(3) is False)
		assert(q.has(4) is False)

		assert(q.pop(msg_index1) == 1)
		msg_index1 += 1
		assert(q.count() == 2)
		assert(q.has(0) is False)
		assert(q.has(1) is True)
		assert(q.has(2) is True)
		assert(q.has(3) is False)
		assert(q.has(4) is False)

		q.unsubscribe(msg_index1)
		assert(q.count() == 2)
		assert(q.has(0) is False)
		assert(q.has(1) is True)
		assert(q.has(2) is True)
		assert(q.has(3) is False)
		assert(q.has(4) is False)

		q.unsubscribe(msg_index2)
		assert(q.count() == 0)
		assert(q.has(0) is False)
		assert(q.has(1) is False)
		assert(q.has(2) is False)
		assert(q.has(3) is False)
		assert(q.has(4) is False)

		q.push(3)
		assert(q.count() == 0)
		assert(q.has(0) is False)
		assert(q.has(1) is False)
		assert(q.has(2) is False)
		assert(q.has(3) is False)
		assert(q.has(4) is False)

		q.subscribe()
		q.push(3)
		assert(q.count() == 1)
		assert(q.has(0) is False)
		assert(q.has(1) is False)
		assert(q.has(2) is False)
		assert(q.has(3) is True)
		assert(q.has(4) is False)
