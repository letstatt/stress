from random import randint

n = int(input())
a = list(map(int, input().split()))

for i in range(len(a)):
    if a[i] == '1':
        print(i, end=" \t\r\n" * randint(0,2))
