
import pytest
import threading

from wasp_c_extensions.threads import WAtomicCounter, WPThreadEvent, awareness_wait


class TestWAtomicCounter:

	__threads__ = 50
	__repeats__ = 50

	def test(self):

		c = WAtomicCounter()
		assert(c.__int__() == 0)

		c = WAtomicCounter()
		assert(c.__int__() == 0)

		c.increase_counter(1)
		assert(c.__int__() == 1)

		c.increase_counter(10)
		assert(c.__int__() == 11)

		c = WAtomicCounter(5)
		assert(c.__int__() == 5)

		c.increase_counter(1)
		assert(c.__int__() == 6)

	def test_counter_maximum(self):
		u_long_long_bits_count = (8 * 8)

		WAtomicCounter((1 << u_long_long_bits_count) - 1)
		WAtomicCounter((1 << u_long_long_bits_count) + 10)

	def test_multi_threading(self):
		c = WAtomicCounter()

		def thread_fn_increase():
			for i in range(self.__repeats__):
				c.increase_counter(1)

		threads = [threading.Thread(target=thread_fn_increase) for x in range(self.__threads__)]
		for th in threads:
			th.start()

		for th in threads:
			th.join()

		assert(c.__int__() == (self.__threads__ * self.__repeats__))


class TestWPThreadEvent:

	__wait_test_timeout__ = 3

	__threads__ = 50
	__repeats__ = 50

	def test(self):
		event = WPThreadEvent()
		assert(event.is_set() is False)
		event.clear()
		assert(event.is_set() is False)

		event.set()
		assert(event.is_set() is True)

		event = WPThreadEvent(None)
		assert(event.is_set() is False)
		event.clear()
		assert(event.is_set() is False)

		event.set()
		assert(event.is_set() is True)

		assert(event.wait() is True)
		assert(event.wait(None) is True)

		event.set()
		assert(event.is_set() is True)

		event.clear()
		assert(event.is_set() is False)

		assert(event.wait(self.__wait_test_timeout__) is False)
		assert(event.is_set() is False)

	def test_concurrency(self):
		event = WPThreadEvent()

		def threading_fn():
			event.wait()

		threads = [threading.Thread(target=threading_fn) for _ in range(10)]

		for th in threads:
			th.start()

		event.set()

		for th in threads:
			th.join()

	def test_multi_threading(self):

		self.test_counter = 0

		events = [WPThreadEvent() for _ in range(self.__threads__)]

		def threading_fn_gen(wait_event_obj, set_event_obj):
			def threading_fn():
				for _ in range(self.__repeats__):
					wait_event_obj.wait()
					wait_event_obj.clear()
					self.test_counter += 1
					set_event_obj.set()
			return threading_fn

		threads = [
			threading.Thread(target=threading_fn_gen(
				events[i], events[(i + 1) % self.__threads__]
			)) for i in range(self.__threads__)
		]

		for th in threads:
			th.start()

		events[0].set()

		for th in threads:
			th.join()

		assert(self.test_counter == (self.__threads__ * self.__repeats__))


@pytest.mark.parametrize("event_cls", (WPThreadEvent, threading.Event))
def test_awareness(event_cls):

	class A:

		def __init__(self):
			self.a = True

		def __call__(self, *args, **kwargs):
			return self.a

	a = A()

	event = event_cls()
	assert(event.is_set() is False)

	assert(awareness_wait(event, a) is True)
	assert(event.is_set() is True)

	a.a = False
	assert(awareness_wait(event, a, timeout=0.5) is False)
	assert(event.is_set() is False)

	th = threading.Thread(target=lambda: awareness_wait(event, a))
	th.start()
	assert(a.a is False)
	assert(event.is_set() is False)
	event.set()
	th.join()
	assert(event.is_set() is True)
	assert(a.a is False)

	a.a = True
	assert(awareness_wait(event, a) is True)
	assert(event.is_set() is True)
