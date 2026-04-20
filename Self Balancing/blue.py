import asyncio
from bleak import BleakClient, BleakScanner

async def connect_and_send():
    print("Scanning for Bluetooth devices...")
    
    devices = await BleakScanner.discover()
    target_name = "My local bt"
    target_device = None
    
    print(f"Found {len(devices)} devices")
    for device in devices:
        print(f"Device: {device.name or 'Unknown'} - {device.address}")
        if device.name == target_name:
            target_device = device
            print(f"Found target device: {target_name} at {device.address}")
            break
    
    if target_device:
        try:
            print(f"Connecting to {target_device.address}...")
            
            async with BleakClient(target_device.address) as client:
                print(f"Connected to {target_device.name or target_device.address}")
                
                # Discover all services and characteristics
                print("Discovering services and characteristics...")
                for service in client.services:
                    print(f"Service: {service.uuid}")
                    for char in service.characteristics:
                        print(f"  Characteristic: {char.uuid}")
                        print(f"    Properties: {', '.join(char.properties)}")
                
                # You can look for characteristics with 'write' or 'write-without-response' properties
                # Then use that UUID here
                characteristic_uuid = "beb5483e-36e1-4688-b7f5-ea07361b26a8"  # Replace with actual UUID
                
                # Send data
                print(f"Sending data to characteristic {characteristic_uuid}...")
                await client.write_gatt_char(characteristic_uuid, b"hey")
                print("Message sent successfully")
                
        except Exception as e:
            print(f"Error: {e}")
    else:
        print(f"Device '{target_name}' not found")
        
        # If target device wasn't found by name, print all discovered devices
        print("\nAll discovered devices:")
        for i, device in enumerate(devices):
            print(f"{i+1}. {device.name or 'Unknown'} - {device.address}")

# Run the async function
if __name__ == "__main__":
    asyncio.run(connect_and_send())