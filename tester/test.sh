gcc Compress.c LZWCmp.c SmartAlloc.c CodeSet.c

./Compress loremIpsum.staley test1.staley test2.staley toCompress.staley testOne.staley testEmpty.staley
./a.out loremIpsum.me test1.me test2.me toCompress.me testOne.me testEmpty.me
echo one
diff loremIpsum.me.Z loremIpsum.staley.Z
echo two
diff test1.me.Z test1.staley.Z
echo three
diff test2.me.Z test2.staley.Z
echo four
diff toCompress.me.Z toCompress.staley.Z
echo five
diff testOne.me.Z testOne.staley.Z
echo six
diff testEmpty.me.Z testEmpty.staley.Z

rm loremIpsum.me.Z loremIpsum.staley.Z test1.me.Z test1.staley.Z test2.me.Z test2.staley.Z toCompress.me.Z toCompress.staley.Z testOne.me.Z testOne.staley.Z testEmpty.me.Z testEmpty.staley.Z

echo seven
./Compress -tcbsr loremIpsum.me > loremIpsum.staley.out
./a.out -tcbsr loremIpsum.me > loremIpsum.me.out
diff loremIpsum.me.out loremIpsum.staley.out
rm loremIpsum.me.out loremIpsum.staley.out

# takes up too much space
#./Compress -tcbsr test1.me > test1.staley.out
#./a.out -tcbsr test1.me > test1.me.out
#diff test1.me.out test1.staley.out
#rm test1.me.out test1.staley.out

echo eight
./Compress -tcbsr test2.me > test2.staley.out
./a.out -tcbsr test2.me > test2.me.out
diff test2.me.out test2.staley.out
rm test2.me.out test2.staley.out

echo nine
./Compress -cbsr toCompress.me > toCompress.staley.out
./a.out -cbsr toCompress.me > toCompress.me.out
diff toCompress.me.out toCompress.staley.out
rm toCompress.me.out toCompress.staley.out
