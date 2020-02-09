/*
Author: Eleanor Gregory
Date: Oct 2019

Unit tests for methods created as part of Coursework 1.

/ᐠ .ᆺ. ᐟ\ﾉ

*/

#include "pch.h"
#include "CppUnitTest.h"
#include "..\Coursework1\Coursework1.cpp"
#include "..\Coursework1\Puzzle.cpp"
#include "..\Coursework1\FileHandler.cpp"
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CW1UnitTests
{
	TEST_CLASS(CW1UnitTests)
	{
	public:

		TEST_METHOD_INITIALIZE(InitialiseTests)
		{
			int blocks[] = { 1, 2, 3, 4, 5, 6, 7, 8, 0 };
			Puzzle p(blocks, 3);
			FileHandler fh("UnitTesting.txt");
			fh.open(WRITE);
			fh.write(p);
			fh.close();
		}

		/* PUZZLE CLASS TESTS */

		TEST_METHOD(CreatePuzzleWithSizeOnly)
		{
			int expectedSize = 3;
			int expectedNoOfBlocks = 9;

			Puzzle p(3);

			Assert::AreEqual(p.get_puzzle_size(), expectedSize);
			Assert::AreEqual(p.get_no_of_blocks(), expectedNoOfBlocks);

		}

		TEST_METHOD(CreatePuzzleWithSizeAndBlocks)
		{
			int expectedSize = 3;
			int expectedNoOfBlocks = 9;
			int blocks[] = { 1, 2, 3, 4, 5, 6, 7, 8, 0 };

			Puzzle p(blocks, 3);

			Assert::AreEqual(p.get_puzzle_size(), expectedSize);
			Assert::AreEqual(p.get_no_of_blocks(), expectedNoOfBlocks);

			int* setBlocks = p.get_all_blocks();

			for (int i = 0; i < expectedNoOfBlocks; i++)
			{
				Assert::AreEqual(setBlocks[i], blocks[i]);
			}
		}

		TEST_METHOD(SetAndGetAllPuzzleBlocks)
		{
			Puzzle p(3);
			int expectedNoOfBlocks = 9;
			int blocks[] = { 1, 2, 3, 4, 5, 6, 7, 8, 0 };

			p.set_all_blocks(blocks);
			int* setBlocks = p.get_all_blocks();
			for (int i = 0; i < expectedNoOfBlocks; i++)
			{
				Assert::AreEqual(setBlocks[i], blocks[i]);
			}
		}

		TEST_METHOD(SetAndGetPuzzleBlocksAsMatrix)
		{
			Puzzle p(3);
			int blockCounter = 1;
			for (int i = 0; i < 3; i++)
			{
				for (int j = 0; j < 3; j++)
				{
					p.set_block_value(blockCounter, j, i);
					blockCounter++;
				}
			}

			int testCount = 1;
			for (int i = 0; i < 3; i++)
			{
				for (int j = 0; j < 3; j++)
				{
					Assert::AreEqual(testCount, p.get_block_value(j, i));
					testCount++;
				}
			}

		}

		TEST_METHOD(ConvertPuzzleBlocksToMatrix)
		{
			int expectedSize = 3;
			int expectedNoOfBlocks = 9;
			int blocks[] = { 1, 2, 3, 4, 5, 6, 7, 8, 0 };

			Puzzle p(blocks, 3);

			p.convert_to_matrix();
			int testCount = 0;
			for (int i = 0; i < 3; i++)
			{
				for (int j = 0; j < 3; j++)
				{
					Assert::AreEqual(blocks[testCount], p.get_block_value(j, i));
					testCount++;
				}
			}
		}


		/* HELPER FUNCTION TESTS */
		TEST_METHOD(TestRowIsContinuous)
		{
			int blocks[] = { 1, 2, 3 };
			int size = 3;

			bool result = check_cont(blocks, size);

			Assert::IsTrue(result);
		}

		TEST_METHOD(TestRowIsntContinuous)
		{
			int blocks[] = { 1, 5, 3 };
			int size = 3;

			bool result = check_cont(blocks, size);

			Assert::IsFalse(result);
		}

		TEST_METHOD(TestRowIsReverseContinuous)
		{
			int blocks[] = { 1, 2, 3 };
			int size = 3;

			bool result = check_rev_cont(blocks, size);

			Assert::IsTrue(result);
		}

		TEST_METHOD(TestRowIsntReverseContinuous)
		{
			int blocks[] = { 3, 5, 1 };
			int size = 3;

			bool result = check_rev_cont(blocks, size);

			Assert::IsFalse(result);
		}

		TEST_METHOD(TestFactorialIsCorrect)
		{
			int factorialToDo = 5;
			int expected = 120;

			int result = small_factorial(factorialToDo);
			Assert::AreEqual(expected, result);
		}	

		TEST_METHOD(CheckAreOnSameRow)
		{
			vector<int> testIndexes = { 0, 1, 2 };
			Assert::IsTrue(same_row_check(testIndexes,3));
		}

		TEST_METHOD(CheckOnDifferentRow)
		{
			vector<int> testIndexes = { 1, 2, 3 };
			Assert::IsFalse(same_row_check(testIndexes,3));
		}

		TEST_METHOD(CheckOnSameColumn)
		{
			vector<int> testIndexes = { 0, 3, 6 };
			Assert::IsTrue(same_column_check(testIndexes, 3));
		}

		TEST_METHOD(CheckOnDifferentColumn)
		{
			vector<int> testIndexes = { 1, 2, 3 };
			Assert::IsFalse(same_column_check(testIndexes, 3));
		}

		TEST_METHOD(TwoPartialFindingForFourByFourPuzzleSingleConfig)
		{
			int blocks[16] = { 1,5,7,8,2,6,11,12,3,9,13,14,4,10,15,0 };
			Puzzle p(blocks, 4);

			int expectedTwos = 8;

			PartialStats results = { 0 };
			conts_for_single_config(p, 2, &results.twos);
			Assert::AreEqual(expectedTwos, results.twos);
		}

		TEST_METHOD(ThreePartialFindingForFourByFourPuzzleSingleConfig)
		{
			int blocks[16] = { 1,5,7,8,2,6,11,12,3,9,13,14,4,10,15,0 };
			Puzzle p(blocks, 4);

			int expectedThrees = 2;

			PartialStats results = { 0 };
			conts_for_single_config(p, 3, &results.threes);
			Assert::AreEqual(expectedThrees, results.threes);
		}

		TEST_METHOD(FourPartialFindingForFourByFourPuzzleSingleConfig)
		{
			int blocks[16] = { 1,5,7,8,2,6,11,12,3,9,13,14,4,10,15,0 };
			Puzzle p(blocks, 4);

			int expectedFours = 1;

			PartialStats results = { 0 };
			conts_for_single_config(p, 4, &results.fours);
			Assert::AreEqual(expectedFours, results.fours);
		}

		/* FILE IO TESTS */
		TEST_METHOD(OpenAFileForWritingTo)
		{
			FileHandler fh("UnitTesting.txt");
			fh.open(WRITE);

			Assert::IsTrue(fh.write_is_open());
		};

		TEST_METHOD(OpenAFileForReadingFrom)
		{
			FileHandler fh("UnitTesting.txt");
			fh.open(READ);

			Assert::IsTrue(fh.read_is_open());
		};

		TEST_METHOD(WriteAnIntToAFileAndReadItBack)
		{
			int toWrite = 10;
			FileHandler fh("UnitTesting.txt");
			fh.open(WRITE);

			fh.write(toWrite);

			fh.switch_mode(READ);

			int result;
			fh.read_int(result);

			Assert::AreEqual(result, toWrite);


			fh.close();
		}

		TEST_METHOD(SwitchFileModeFromWriteToReadAndBack)
		{
			FileHandler fh("UnitTesting.txt");
			fh.open(WRITE);

			Assert::IsTrue(fh.write_is_open());
			Assert::IsFalse(fh.read_is_open());

			fh.switch_mode(READ);
			Assert::IsFalse(fh.write_is_open());
			Assert::IsTrue(fh.read_is_open());

			fh.switch_mode(WRITE);
			Assert::IsTrue(fh.write_is_open());
			Assert::IsFalse(fh.read_is_open());

			fh.close();
		}

		TEST_METHOD(ReadInFirstLineOfFile)
		{
			FileHandler fh("UnitTesting.txt");
			fh.open(READ);
			string expected = "1\t2\t3\t";
			string result;
			fh.read_line(result);

			Assert::AreEqual(result, expected);

			fh.close();
		}

		TEST_METHOD(ClearFile)
		{
			FileHandler fh("UnitTesting.txt");
			fh.clear();

			string out; 
			string expected = "";
			fh.read_line(out);
			Assert::AreEqual(expected, out);

			fh.close();
		}



	};
}
