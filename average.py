def calculate_average(file_name):
    total = 0
    count = 0
    with open(file_name, 'r') as file:
        for line in file:
            columns = line.split()
            if len(columns) >= 2:
                try:
                    value = float(columns[1])
                    total += value
                    count += 1
                except ValueError:
                    print("Skipping non-numeric value in the second column.")
    if count > 0:
        average = total / count
        return average
    else:
        return None

file_name = input("Enter the file name: ")
average = calculate_average(file_name)
if average is not None:
    print("The average of values in the second column is:", average)
else:
    print("No valid numerical values found in the second column.")
