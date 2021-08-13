
import pytest
import threading

from wasp_c_extensions.threads import WAtomicCounter, WPThreadEvent, awareness_wait
from wasp_c_extensions.threads import LT_test, LE_test, EQ_test, NE_test, GT_test, GE_test


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

	def test_negative(self):
		WAtomicCounter()
		WAtomicCounter(-7)
		WAtomicCounter(-7, negative=True)
		pytest.raises(ValueError, WAtomicCounter, -7, negative=False)

		c = WAtomicCounter(negative=False)
		assert(int(WAtomicCounter()) == 0)

		c = WAtomicCounter(3, negative=False)
		assert(int(c) == 3)
		pytest.raises(ValueError, c.increase_counter, -10)
		assert(int(c) == 3)

	def test_set(self):
		counter = WAtomicCounter()
		assert(counter.set(5) == 0)
		assert(int(counter) == 5)

		counter = WAtomicCounter(1, negative=False)
		assert(counter.set(7) == 1)
		assert(int(counter) == 7)

		pytest.raises(ValueError, counter.set, -3)

	__tas_parameters__ = [
		(LT_test, 3, -1, None, 3),
		(LT_test, 3, 3, None, 3),
		(LT_test, 3, 5, 3, 5),

		(LE_test, 3, -1, None, 3),
		(LE_test, 3, 3, 3, 3),
		(LE_test, 3, 5, 3, 5),

		(EQ_test, 3, -1, None, 3),
		(EQ_test, 3, 3, 3, 3),
		(EQ_test, 3, 5, None, 3),

		(NE_test, 3, -1, 3, -1),
		(NE_test, 3, 3, None, 3),
		(NE_test, 3, 5, 3, 5),

		(GT_test, 3, -1, 3, -1),
		(GT_test, 3, 3, None, 3),
		(GT_test, 3, 5, None, 3),

		(GE_test, 3, -1, 3, -1),
		(GE_test, 3, 3, 3, 3),
		(GE_test, 3, 5, None, 3),
	]

	@pytest.mark.parametrize("test_op, start_value, compare_value, tas_result, counter_result", __tas_parameters__)
	def test_tas(self, test_op, start_value, compare_value, tas_result, counter_result):
		counter = WAtomicCounter(start_value)
		tas = counter.test_and_set(test_op, WAtomicCounter(compare_value))
		assert((tas is tas_result) or (tas == tas_result))
		assert(int(counter) == counter_result)

	@pytest.mark.parametrize("test_op", [LT_test, LE_test, EQ_test, NE_test, GT_test, GE_test])
	def test_tas_exceptions(self, test_op):
		pytest.raises(
			ValueError,
			WAtomicCounter(0, negative=False).test_and_set,
			test_op,
			WAtomicCounter(-1)
		)

	def test_cas(self):
		counter = WAtomicCounter(1)
		assert(counter.compare_and_set(WAtomicCounter(5), WAtomicCounter(7)) is None)
		assert(int(counter) == 1)

		assert(counter.compare_and_set(WAtomicCounter(1), WAtomicCounter(-7)) == 1)
		assert(int(counter) == -7)

		counter = WAtomicCounter(3, negative=False)
		pytest.raises(ValueError, counter.compare_and_set, WAtomicCounter(3), WAtomicCounter(-5))
		assert(int(counter) == 3)


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
