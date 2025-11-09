from dataclasses import dataclass, asdict
import csv
import os
from typing import Any, Dict, List, Optional


@dataclass 
class Item:
    name : str
    count : int
    meta : str

@dataclass
class Effect:
    name : str
    type : str
    duration : float
    amplifierLevel : int


@dataclass
class DataSnap:
    fps: float
    time: str
    date: str
    plyrName: str
    plyrLocation: List[float]
    plyrHealth: float
    plyrInventory: List[Item]
    plyrArmor: str
    plyrOffhand: str
    plyrStatus: List[Effect]
    plyrHunger: float
    plyrSat: float
    plyrView: List[float]
    plyrFacing: str
    plyrSelectedSlot: int
    plyrSelectedItem: str
    plyrRideState: bool
    plyrRideVehicle: str
    plyrMomentum: float 

DEFAULT_DATA_SNAP = DataSnap(69.0, "22:23:03.760143900", "2025-01-11", "Playername", [1.0, 3.0, 4.0], 20.0, [Item("Example", 1, "")], "Armor", "Offhand", 
                           [Effect("ExampleEffect", "minecraft:example", 12.0, 2)], 10.0, 10.0, [1.0,1.0,1.0],"(east,)", 1, "Birch", False, "None", 10.0)

def string_to_bool(value):
    truthy_values = {"true", "1", "yes", "on"}
    falsy_values = {"false", "0", "no", "off"}
    
    if not isinstance(value, str):
        value = str(value)

    # Normalize the input
    value = value.strip().lower()
    
    if value in truthy_values:
        return True
    elif value in falsy_values:
        return False
    else:
        raise ValueError(f"Cannot convert '{value}' to a boolean")
def cleanSplit( source: str, token: str)  -> List[str]:
    return [s.strip() for s in source.split(token) if s.strip() != '']

"""
def pairwiseGenerator(listObj):
    it = iter(listObj)
    one = next(it, None)  # Get the first element
    two = next(it, None)  # Get the first element
    while one != None and two != None:
        yield one, two
        one = next(it, None)  # Get the first element
        two = next(it, None)  # Get the first element
"""

def decryptInv(plyrInventory : str) -> List[Item]:
    out: List[Item] = []
    if not plyrInventory or plyrInventory.strip() == 'None':
        return out
    inventory_parts = cleanSplit(plyrInventory, ";")
    for pI in inventory_parts:
        try:
            parts = pI.split(":")
            if len(parts) < 2: continue
            itemName = parts[0].strip()
            itemCount = parts[1].strip() if len(parts) > 2 else "0"
            out.append(Item(name = itemName, count = int(itemCount.strip()), meta = ""))
        except (ValueError, IndexError):
            continue
    return out

def decryptStatus(status : str) -> List[Effect]:
    out = []
    if not status or status.strip() == 'None':
        return out
    else:
        for stat in cleanSplit(status, ";"):
            try:
                fragmentVals = []
                for fragment in cleanSplit(stat, ','):
                    val = cleanSplit(fragment, ":")
                    if len(val) > 1:
                        fragmentVals.append(val[1])
                if len(fragmentVals) >= 4:

                    out.append(
                        Effect( name = fragmentVals[0], 
                                type = fragmentVals[1], 
                                duration= float(fragmentVals[2]),
                                amplifierLevel= int(fragmentVals[3])
                        )
                    )
            except (ValueError, IndexError):
                continue
    return out

def decrypt(data: Dict[str,Any]) -> Optional[DataSnap]: # Takes dict as input, decrypts and returns the data class
    try:
        date = str(data.get('date',''))
        fps = float(data.get("fps",0.0))
        time = str(data.get('time',''))
        plyrName = str(data.get('plyrName',''))

        plyrInventory_str = str(data.get('plyrInventory',''))
        plyrInventory = decryptInv(plyrInventory_str)

        plyrArmor = str(data.get('plyrArmor',''))
        plyrOffhand = str(data.get('plyrOffhand',''))

        plyrStatus_str = str(data.get('plyrStatus',''))
        plyrStatus = decryptStatus(plyrStatus_str)

        plyrLocation = eval(data.get('plyrLocation','[]'))
        plyrHealth = float(data.get('plyrHealth',0.0))
        plyrHunger = float(data.get('plyrHunger',0.0))
        plyrSat = float(data.get('plyrSat',0.0))
        plyrView = eval(data.get('plyrView','[]'))
        plyrFacing = str(data.get('plyrFacing'))
        plyrSelectedSlot = int(data.get('plyrSelectedSlot',0))
        plyrSelectedItem = str(data.get('plyrSelectedItem',''))
        plyrRideState = string_to_bool(data.get('plyrRideState','false'))
        plyrRideVehicle = str(data.get('plyrRideVehicle',''))
        plyrMomentum = float(data.get('plyrMomentum',0.0))

        return DataSnap(
            date = date,
            fps = fps,
            time = time,
            plyrName = plyrName,
            plyrInventory = plyrInventory,
            plyrArmor = plyrArmor,
            plyrOffhand = plyrOffhand,
            plyrStatus = plyrStatus,
            plyrLocation = plyrLocation,
            plyrHealth = plyrHealth,
            plyrHunger = plyrHunger,
            plyrSat = plyrSat,
            plyrView = plyrView,
            plyrFacing = plyrFacing,
            plyrSelectedSlot = plyrSelectedSlot,
            plyrSelectedItem = plyrSelectedItem,
            plyrRideState = plyrRideState,
            plyrRideVehicle = plyrRideVehicle,
            plyrMomentum = plyrMomentum
        )
    except (ValueError, TypeError,SyntaxError) as e:
        print(f'Decryption failed with error: {e}')
        print(f"Problematic data: {data}")
        return None
def save_to_csv(data: DataSnap, filename:str):
    data_dict = asdict(data)
    # Open a CSV file to write the data
    with open(filename, mode='a', newline="") as file:
        writer = csv.DictWriter(file,fieldnames=data_dict.keys())

        if os.path.exists(filename) and os.stat(filename).st_size == 0:
            writer.writeheader()

        writer.writerow(data_dict)

def load_from_csv(filename:str) -> List[DataSnap]:
    with open(filename, mode="r") as file:
        reader = csv.DictReader(file)
        # Convert each row in the csv file to a dictionary which is then added to a list of dictionaries
        #print(dataDicts)
        outputData:List[DataSnap] = []
        # The code noted below may or may not be needed
        for dataDict in reader:
            datasnap_instance = decrypt(dataDict)
            if datasnap_instance is not None:
                outputData.append(datasnap_instance)
        return outputData

"""
## Deprecated
def save_to_table(dataObjects,table_name):
    if(isinstance(dataObjects, DataSnap)):
        dataObjects = asdict(dataObjects)
        dataObjectsList = [dataObjects]
    elif(isinstance(dataObjectsList, list)):
        pass
    else:
        return # not valid

    for dataObj in dataObjects:
        #print(data)
        #data_dict = asdict(dataObj)
        db.create_table(table_name,dataObj) #data_dict#)
        db.insert_Dataframe(table_name,dataObj) #data_dict)
"""

"""
## Deprecated
# TODO split dataframe into main table
def save_dataframe_to_database(dataframe):
    data_table_entry = ""
    effects_table_entry = dataframe.plyrStatus
    item_table_entry = dataframe.plyrInventory # maybe we need to specify item inventory location as well!
"""

def test1():
    """test stuff"""
    print("testing data format decrypt")
    print("\n\neg. plyrInv...:")
    nameCount = {
        "bub" : "123",
        "nub1" : "230",
        "moob" : "45",
        "rob" : "93"
    }
    pI = ''
    for idx, i in enumerate(nameCount.items()):
        pI += f"Main Inventory {idx}: {i[0]}, Count: {i[1]}; "
    print(pI, "\n => \n", decryptInv(pI))

def test2():
    data = load_from_csv("../saves/playerData_01_11_2025_22_23_01.csv")
    print("data", data[0])
    #save_to_table(data, "DATA")
def test3():
    #q = "SELECT tablename, schemaname FROM pg_catalog.pg_tables WHERE tablename = 'data';"
    #db.custom_command(q)
    print("deprecated")
""" if u run this file standalone it will simply test some stuff"""
if __name__ == "__main__":
    #db.createDatabase()
    test2()
    #test3()
    pass