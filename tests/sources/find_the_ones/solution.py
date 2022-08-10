n = int(input())
a = list(map(int, input().split()))
b = []

for i in range(len(a)):
    if a[i] == '1':
        b.append(i)

print(*b)
