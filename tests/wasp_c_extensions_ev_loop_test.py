
from time import sleep
from random import random
from threading import Thread

import pytest

from wasp_c_extensions.threads import WPThreadEvent
from wasp_c_extensions.ev_loop import WEventLoop
# from wasp_c_extensions.cmcqueue import WCMCQueue, WCMCQueueItem


class TestEventLoopConcurrency:
	__test_running__ = False
	__threads_count__ = 50
	__input_sequence__ = [int(random() * 10) for _ in range(10 ** 4)]
	__wait_pause__ = 0.01

	def test(self):
		loop = WEventLoop()
		thread = Thread(target=loop.start_loop)
		thread.start()
		loop.stop_loop()
		thread.join()

	@pytest.mark.parametrize('runs', range(5))
	def test_concurrency(self, runs):
		loop = WEventLoop(immediate_stop=False)
		loop_thread = Thread(target=loop.start_loop)
		loop_thread.start()

		result = []

		class Callback:
			def __init__(self, i):
				self.i = i

			def __call__(self, *args, **kwargs):
				result.append(self.i)

		def pub_thread_fn():
			for i in TestEventLoopConcurrency.__input_sequence__:
				loop.notify(Callback(i))

		pub_threads = [Thread(target=pub_thread_fn) for _ in range(TestEventLoopConcurrency.__threads_count__)]
		for i in pub_threads:
			i.start()

		for i in pub_threads:
			i.join()

		loop.stop_loop()
		loop_thread.join()

		target_result = sum(TestEventLoopConcurrency.__input_sequence__) * TestEventLoopConcurrency.__threads_count__
		assert(sum(result) == (target_result))