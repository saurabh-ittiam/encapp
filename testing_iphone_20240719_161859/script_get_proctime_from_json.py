import json
import csv
import sys
import os

def process_file(input_json_file, prefix):
    # Load the JSON data
    with open(input_json_file, 'r') as json_file:
        data = json.load(json_file)
    
    # Extract sourcefile name
    sourcefile = data.get('sourcefile', '')
    if not sourcefile:
        print(f"Error: 'sourcefile' key not found in JSON file: {input_json_file}")
        return
    
    # Construct output CSV filename with prefix and sourcefile name
    output_csv_file = f"{prefix}_{os.path.splitext(sourcefile)[0]}_proctime.csv"

    # Check if the root element is a dictionary and contains a key with frames
    if isinstance(data, dict):
        frames = data.get('frames', [])
    elif isinstance(data, list):
        frames = data
    else:
        print(f"Unsupported JSON format in file: {input_json_file}")
        return
    
    # Extract proctime values
    proctimes = [frame.get('proctime', 0) for frame in frames]

    # Calculate the sum and average of proctime values
    total_proctime = sum(proctimes)
    average_proctime = total_proctime / len(proctimes) if proctimes else 0

    # Write proctime values to CSV
    with open(output_csv_file, 'w', newline='') as csv_file:
        csv_writer = csv.writer(csv_file)
        
        # Write header
        csv_writer.writerow(['proctime'])
        
        # Write proctime values
        for proctime in proctimes:
            csv_writer.writerow([proctime])
        
        # Write sum and average
        csv_writer.writerow([])
        csv_writer.writerow(['total_proctime', total_proctime])
        csv_writer.writerow(['average_proctime', average_proctime])

    print(f'CSV file created successfully: {output_csv_file}')

def main():
    if len(sys.argv) < 3:
        print(f"Usage: {sys.argv[0]} <prefix> <input_json_file1> [<input_json_file2> ...]")
        return

    prefix = sys.argv[1]

    # Process each JSON file
    for i in range(2, len(sys.argv)):
        input_json_file = sys.argv[i]
        process_file(input_json_file, prefix)

if __name__ == '__main__':
    main()
