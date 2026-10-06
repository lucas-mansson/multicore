{-# OPTIONS_GHC -Wno-unrecognised-pragmas #-}
{-# HLINT ignore "Use camelCase" #-}
import Control.Parallel
import Control.DeepSeq
import Data.Time
import Text.Printf (printf)

int_isqrt = floor . sqrt . fromIntegral

check_is_prime::(Int,Int)->Bool
check_is_prime(a, n) =
	if a == 1+int_isqrt n then
		True
	else if n `mod` a == 0 then
		False
	else
		check_is_prime(a+1, n)
		
is_prime:: Int->Bool
is_prime(n) = 
	if n == 2 then True
	else
		check_is_prime(2, n)

seq_sum_primes:: (Int,Int)->Int
seq_sum_primes(a, b) =
	if a > b then 0
	else if (is_prime(b)) then 	
		b + seq_sum_primes(a, b-1)
	else	
		seq_sum_primes(a, b-1)

par_sum_primes:: Int -> (Int,Int)->Int
par_sum_primes nbr_threads (a, b) = split_range nbr_threads a b
	
split_range :: Int -> Int -> Int -> Int
split_range k lo hi
	| hi < lo  = 0
	| k <= 1   = seq_sum_primes(lo, hi)
	| otherwise =
		let	mid   = lo + (hi - lo) `div` 2
			half  = k `div` 2
			left  = split_range half lo mid
			right = split_range (k - half) (mid + 1) hi
		in	par left (pseq right (left + right))


main:: IO()
main = do
	line <- getLine

	begin <- getCurrentTime

	let	[n] = map read (words line) :: [Int]
		s = par_sum_primes 10 (2,n)

	s `deepseq` do
		end <- getCurrentTime

		printf "par haskell sum of primes in 2..%d = %d \n" n s
		printf "time: "
		print $ diffUTCTime end begin
