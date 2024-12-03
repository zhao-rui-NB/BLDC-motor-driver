def calculate_checksum(byte_input):
    # Remove spaces and convert to bytes
    byte_input = byte_input.replace(" ", "")
    byte_data = bytes.fromhex(byte_input)
    
    # Calculate checksum by XORing all bytes except the last one
    checksum = 0
    for i in range(len(byte_data)):
        checksum ^= byte_data[i]
    
    return checksum

# Main loop for input and checksum calculation
while True:
    try:
        byte = input("Enter byte: ")
        result = calculate_checksum(byte)
        print(f"Checksum: 0x{result:02X}")
    except ValueError as e:
        print(f"Error: {e}")
    except KeyboardInterrupt:
        print("\nExiting program.")
        break