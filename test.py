def computer_area(radius):
    pi = 3.14159
    area = pi * radius * radius
    print(f'Address of pi inside function: {hex(id(pi))}')
    return area

r = 5
result = computer_area(r)
print(f'Address of result 1: {hex(id(result))}')

result = computer_area(r)
print(f'Address of result 2: {hex(id(result))}')
