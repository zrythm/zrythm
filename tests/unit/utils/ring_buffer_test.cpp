// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/gtest_wrapper.h"
#include "utils/ring_buffer.h"

TEST (RingBufferTest, BasicOperations)
{
  RingBuffer<int> buffer (4);
  EXPECT_EQ (buffer.capacity (), 4);

  EXPECT_TRUE (buffer.write (1));
  EXPECT_TRUE (buffer.write (2));

  int value;
  EXPECT_TRUE (buffer.read (value));
  EXPECT_EQ (value, 1);
  EXPECT_TRUE (buffer.read (value));
  EXPECT_EQ (value, 2);
  EXPECT_FALSE (buffer.read (value));
}

TEST (RingBufferTest, ForceWrite)
{
  RingBuffer<int> buffer (2);
  buffer.force_write (1);
  buffer.force_write (2);
  buffer.force_write (3); // Should overwrite oldest value

  int value;
  EXPECT_TRUE (buffer.read (value));
  EXPECT_EQ (value, 2);
  EXPECT_TRUE (buffer.read (value));
  EXPECT_EQ (value, 3);
}

TEST (RingBufferTest, MultipleOperations)
{
  RingBuffer<int>  buffer (4);
  std::vector<int> input = { 1, 2, 3, 4 };
  EXPECT_TRUE (buffer.write_multiple (input.data (), input.size ()));

  std::vector<int> output (4);
  EXPECT_TRUE (buffer.read_multiple (output.data (), output.size ()));
  EXPECT_EQ (input, output);
}

TEST (RingBufferTest, ForceWriteMultiple)
{
  RingBuffer<int>  buffer (3);
  std::vector<int> input = { 1, 2, 3, 4 };
  buffer.force_write_multiple (input.data (), input.size ());

  std::vector<int> output (3);
  EXPECT_TRUE (buffer.read_multiple (output.data (), 3));
  EXPECT_EQ (output, std::vector<int> ({ 2, 3, 4 }));
}

TEST (RingBufferTest, PeekOperations)
{
  RingBuffer<int> buffer (4);
  buffer.write (1);
  buffer.write (2);

  int value;
  EXPECT_TRUE (buffer.peek (value));
  EXPECT_EQ (value, 1);

  std::vector<int> peek_values (2);
  EXPECT_EQ (buffer.peek_multiple (peek_values.data (), 2), 2);
  EXPECT_EQ (peek_values[0], 1);
  EXPECT_EQ (peek_values[1], 2);
}

TEST (RingBufferTest, MultiThreaded)
{
  RingBuffer<int>   buffer (1024);
  std::atomic<bool> done{ false };
  std::atomic<int>  sum{ 0 };

  // Producer thread
  std::thread producer ([&] () {
    for (int i = 1; i <= 1000; i++)
      {
        while (!buffer.write (i))
          {
            std::this_thread::yield ();
          }
      }
    done = true;
  });

  // Consumer thread
  std::thread consumer ([&] () {
    int value;
    while (!done || buffer.read (value))
      {
        if (buffer.read (value))
          {
            sum += value;
          }
      }
  });

  producer.join ();
  consumer.join ();

  EXPECT_GT (sum, 0);
}
