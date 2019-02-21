
import pytest
import gc
from random import random
from threading import Thread

from wasp_c_extensions.threads import WPThreadEvent
from wasp_c_extensions.queue import WMCQueue, WMCQueueSubscriber


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


class TestWMCQueueSubscriber:

	def test(self):
		queue = WMCQueue()
		s1 = WMCQueueSubscriber(queue)
		assert(s1.subscribed() is True)

		assert(s1.has_next() is False)
		pytest.raises(KeyError, s1.next)
		assert(queue.has(0) is False)
		assert(queue.has(1) is False)
		assert(queue.has(2) is False)
		assert(queue.has(3) is False)
		assert(queue.has(4) is False)

		queue.push(1)
		assert(s1.has_next() is True)
		assert(queue.has(0) is True)
		assert(queue.has(1) is False)
		assert(queue.has(2) is False)
		assert(queue.has(3) is False)
		assert(queue.has(4) is False)

		assert(s1.next() == 1)
		assert(queue.has(0) is False)
		assert(queue.has(1) is False)
		assert(queue.has(2) is False)
		assert(queue.has(3) is False)
		assert(queue.has(4) is False)
		pytest.raises(KeyError, s1.next)

		queue.push(2)
		assert(queue.has(0) is False)
		assert(queue.has(1) is True)
		assert(queue.has(2) is False)
		assert(queue.has(3) is False)
		assert(queue.has(4) is False)
		assert(s1.has_next() is True)

		s1.unsubscribe()
		assert(s1.subscribed() is False)
		assert(queue.has(0) is False)
		assert(queue.has(1) is False)
		assert(queue.has(2) is False)
		assert(queue.has(3) is False)
		assert(queue.has(4) is False)

		s1 = WMCQueueSubscriber(queue)
		assert(queue.has(0) is False)
		assert(queue.has(1) is False)
		assert(queue.has(2) is False)
		assert(queue.has(3) is False)
		assert(queue.has(4) is False)

		queue.push(3)
		assert(queue.has(0) is False)
		assert(queue.has(1) is False)
		assert(queue.has(2) is True)
		assert(queue.has(3) is False)
		assert(queue.has(4) is False)

		s1 = None
		gc.collect()
		assert(queue.has(0) is False)
		assert(queue.has(1) is False)
		assert(queue.has(2) is False)
		assert(queue.has(3) is False)
		assert(queue.has(4) is False)

		queue = WMCQueue(callback=None)
		s1 = WMCQueueSubscriber(queue)
		assert(s1.subscribed() is True)

		assert(s1.has_next() is False)
		pytest.raises(KeyError, s1.next)
		assert(queue.has(0) is False)
		assert(queue.has(1) is False)

		queue.push(1)
		assert(s1.has_next() is True)
		assert(queue.has(0) is True)
		assert(queue.has(1) is False)


class TestConcurrency:

	class Subscriber(WMCQueueSubscriber):

		def __init__(self, queue):
			WMCQueueSubscriber.__init__(self, queue)
			self.event = WPThreadEvent()
			self.result = []

		def thread_fn(self):
			while TestConcurrency.__test_running__ or self.has_next():
				if self.has_next() is False:
					self.event.wait(3)
				if self.has_next():
					self.result.append(self.next())
				self.event.clear()

	__test_running__ = False
	__threads_count__ = 50
	__input_sequence__ = [int(random() * 10) for _ in range(10 ** 4)]

	def test_concurrency(self):

		subscribers = []

		def queue_callback():
			for s in subscribers:
				s.event.set()

		queue = WMCQueue(callback=queue_callback)

		def queue_thread():
			TestConcurrency.__test_running__ = True
			for v in TestConcurrency.__input_sequence__:
				queue.push(v)
			TestConcurrency.__test_running__ = False

		threads = [Thread(target=queue_thread)]
		for i in range(TestConcurrency.__threads_count__):
			s = TestConcurrency.Subscriber(queue)
			subscribers.append(s)
			threads.append(Thread(target=s.thread_fn))

		for th in threads:
			th.start()

		for th in threads:
			th.join()

		for s in subscribers:
			assert(s.result == TestConcurrency.__input_sequence__)
