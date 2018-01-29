--
-- Minimal Development Test
-- Mod: test
--

-- Try out PseudoRandom
pseudo = PseudoRandom(13)
assert(pseudo:next() == 22290)
assert(pseudo:next() == 13854)


