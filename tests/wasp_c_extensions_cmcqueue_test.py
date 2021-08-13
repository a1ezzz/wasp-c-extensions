
from time import sleep
from random import random
from threading import Thread

import pytest
from wasp_c_extensions.cmcqueue import WCMCQueue, WCMCQueueItem


class TestWCMCQueue:

    def test(self):
        q = WCMCQueue()
        assert(q.messages() == 0)

        q.push(1)
        assert(q.messages() == 0)

        token1 = q.subscribe()
        assert(isinstance(token1, WCMCQueueItem) is True)
        assert(q.messages() == 1)
        assert(token1.pull() is None)
        assert(q.messages() == 1)  # there are no changes in the queue, it has the last token event

        q.push('2')
        assert(q.messages() == 2)
        assert(token1.pull() == '2')
        assert(q.messages() == 1)
        assert(token1.pull() is None)

        token2 = q.subscribe()
        assert(q.messages() == 2)

        assert(token1.pull() is None)
        assert(q.messages() == 1)  # all tokens point to the same last message

        q.push(3)
        q.push(4)
        q.push(5)
        assert(token1.pull() == 3)
        assert(token1.pull() == 4)
        assert(token2.pull() == 3)
        assert(token2.pull() == 4)
        assert(token2.pull() == 5)
        assert(token1.pull() == 5)

        token1.unsubscribe()
        assert(q.messages() == 2)  # no clean up process yet
        token2.unsubscribe()
        assert(q.messages() == 0)  # no live tokens anymore
        pytest.raises(RuntimeError, token1.unsubscribe)  # unable to unsubscribe twice
        pytest.raises(RuntimeError, token2.pull)  # unable to pull unsubscribed object

    def test_item(self):
        pytest.raises(RuntimeError, WCMCQueueItem)


class TestWCMCQueueConcurrency:
    __test_running__ = False
    __threads_count__ = 50
    __input_sequence__ = [int(random() * 10) for _ in range(10 ** 4)]
    __wait_pause__ = 0.01

    class Subscriber:

        def __init__(self, queue):
            self.queue = queue
            self.token = queue.subscribe()
            self.result = []

        def thread_fn(self):
            while TestWCMCQueueConcurrency.__test_running__:
                if not self.token.has_next():
                    sleep(TestWCMCQueueConcurrency.__wait_pause__)
                item = self.token.pull()
                if item is not None:
                    self.result.append(item)

            while self.token.has_next():
                item = self.token.pull()
                if item is not None:
                    self.result.append(item)

    @pytest.mark.parametrize('runs', range(5))
    def test_concurrency(self, runs):
        subscribers = []
        queue = WCMCQueue()

        def pub_thread():
            for v in TestWCMCQueueConcurrency.__input_sequence__:
                queue.push(v)

        pub_thread = Thread(target=pub_thread)
        sub_threads = []
        for i in range(TestWCMCQueueConcurrency.__threads_count__):
            s = TestWCMCQueueConcurrency.Subscriber(queue)
            subscribers.append(s)
            sub_threads.append(Thread(target=s.thread_fn))

        TestWCMCQueueConcurrency.__test_running__ = True
        pub_thread.start()
        for th in sub_threads:
            th.start()

        pub_thread.join()
        TestWCMCQueueConcurrency.__test_running__ = False
        for th in sub_threads:
            th.join()

        for s in subscribers:
            assert(s.result == TestWCMCQueueConcurrency.__input_sequence__)
