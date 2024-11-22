"""Python <-> Liberty functions"""

# Python imports
import json
import logging
import os
from collections.abc import Iterable

# External imports
from pyosys import libyosys as ys # pylint:disable=E0401

# Internal imports
from pqhelper import file_newer_than, listify, OPS

# Logger
log = logging.getLogger("LIBERTY")

def liberty2json(filepath):
  """Use liberty2json to convert a Liberty file to JSON"""
  os.makedirs("preqorsor/data/lib2json/", exist_ok=True)
  translated_file = f"preqorsor/data/lib2json/{os.path.basename(filepath)}.json"
  if file_newer_than(filepath, translated_file):
    os.system(f"liberty2json {filepath} {translated_file}")
  return translated_file

def liberty2dict(filepath):
  """Liberty to dictionary (TODO: remove liberty2dict below and rename this to liberty2dict)"""
  return json.load(open(liberty2json(filepath), encoding="utf-8"))

def dict2liberty(liberty, current_group=None, indent=2, debug=False, dbg_logger=None):
  """Convert a dictionary to a Liberty library"""
  # If current_group is None, then this is the top-level group, take library
  if current_group is None:
    liberty, current_group = liberty["library"], "library"
    if debug:
      dbg_logger.info("DEBUG: Exporting library")
      with open("preqorsor/results/liberty.json", "w", encoding="utf-8") as f:
        json.dump(liberty, f, indent=2, sort_keys=True)

  # Retrieve attributes and groups
  def isgroup(x):
    return isinstance(x, (dict, list))
  attributes = {k : v for k, v in liberty.items() if not isgroup(v) and k != "comment"}
  groupss = {k : v if isinstance(v, list) else [v] for k, v in liberty.items() if isgroup(v)}

  # Initialize liberty
  liberty = []

  # Attributes
  for k, v in attributes.items():
    if isinstance(v, tuple):
      text = f'{k} ('
      if isinstance(v[0], Iterable):
        text += " \\\n"
        text += ", \\\n".join([" "* indent + f'"{str(l)[1:-1].strip(",")}"' for l in v])
        text += " \\\n"
      elif isinstance(v[0], (int, float)):
        text += f'"{str(v)[1:-1].strip(",")}"'
      text += ");"
      liberty.append(text)
    elif k == "capacitive_load_unit":
      liberty.append(f"{k} ({v});")
    elif isinstance(v, str):
      liberty.append(f'{k} : "{v}";')
    else:
      liberty.append(f"{k} : {str(v).lower()};")

  # Groups (special groups have no name, e.g., "timing () {")
  special_groups = ["timing", "memory", "memory_read", "memory_write",
                    "internal_power", "leakage_power"]
  for group_name, groups in groupss.items():
    for group in groups:
      # Debug show cell
      if debug and current_group == "cell":
        dbg_logger.info(f"DEBUG: Exporting cell {group_name}")

      # Group name header
      if not attributes:
        liberty.append(f"{current_group} ({group_name}) " + "{")
      elif group_name in special_groups:
        liberty.append(f"{group_name} () " + "{")

      # Add group name
      grouplib = dict2liberty(group, group_name, indent, debug, dbg_logger)

      # Indent
      if not attributes or group_name in special_groups:
        grouplib = [" "*indent + line for line in grouplib.split("\n")]
      else:
        grouplib = grouplib.split("\n")

      # Insert
      liberty.extend(grouplib)

      # Add closing brace
      if not attributes or group_name in special_groups:
        liberty.append("}")

  # Return final Liberty
  return "\n".join(liberty)

def power_unit_multiplier(power_unit): # pragma: no cover
  """Convert Liberty power unit to Watt multiplier"""
  if power_unit == "1W":
    mult = 1
  elif power_unit == "100mW":
    mult = 100e-3
  elif power_unit == "10mW":
    mult = 10e-3
  elif power_unit == "1mW":
    mult = 1e-3
  elif power_unit == "100uW":
    mult = 100e-6
  elif power_unit == "10uW":
    mult = 10e-6
  elif power_unit == "1uW":
    mult = 1e-6
  elif power_unit == "100nW":
    mult = 100e-9
  elif power_unit == "10nW":
    mult = 10e-9
  elif power_unit == "1nW":
    mult = 1e-9
  elif power_unit == "100pW":
    mult = 100e-12
  elif power_unit == "10pW":
    mult = 10e-12
  elif power_unit == "1pW":
    mult = 1e-12
  else:
    log.warning("Unknown power unit: %s", power_unit)
    mult = 0
  return mult

# Liberty output functions
def timingtmpl2lib(liberty, cells): # pragma: no cover
  """Adds templates from timing table to Liberty header"""
  liberty["library"]["preqorsor"]["lu_table_template"] = {}
  for cell in cells.values():
    for pin in list(cell.get("pin", {}).values()) + list(cell.get("bus", {}).values()):
      for timing in listify(pin.get("timing", [])):
        for k, v in timing.items():
          comb_delay_groups = ["cell_rise", "cell_fall", "rise_transition", "fall_transition"]
          if k not in comb_delay_groups or "scalar" in v:
            continue
          for template in v.values():
            width = (len(template["index_1"]), len(template["index_2"]))
            delay_tmpl = f"delay_template_{width[0]}x{width[1]}"
            liberty["library"]["preqorsor"]["lu_table_template"][delay_tmpl] = {
              "variable_1": "input_net_transition",
              "variable_2": "total_output_net_capacitance",
              "index_1": tuple(range(width[0])),
              "index_2": tuple(range(width[1])),
            }

def setup2lib(pin, related_pin, pred, polarity=None):
  """Setup timing constraint to Liberty timing"""
  return {
    "related_pin": related_pin,
    "timing_type": "setup_rising" if polarity else "setup_falling",
    "rise_constraint": {
      "scalar": { "values": (pred.get(f"{related_pin.split('[')[0]}->{pin}.rise_constraint", 0),) }
    },
    "fall_constraint": {
      "scalar": { "values": (pred.get(f"{related_pin.split('[')[0]}->{pin}.fall_constraint", 0),) }
    }
  }

def timing2lib(pin, related_pin, pred, unateness=None, polarity=None):
  """Setup timing delay to Liberty timing"""
  # Initial setup
  timing = {"related_pin": related_pin}
  if unateness:
    timing["timing_sense"] = unateness
  if polarity is not None:
    timing["timing_type"] = "rising_edge" if polarity else "falling_edge"
  if pin == "RD_DATA" and related_pin != "RD_CLK":
    related_pin = "RD_CLK"
  cell_rise = pred.get(f"{related_pin.split('[')[0]}->{pin.split('[')[0]}.cell_rise", 0)
  cell_fall = pred.get(f"{related_pin.split('[')[0]}->{pin.split('[')[0]}.cell_fall", 0)
  rise_transition = pred.get(f"{related_pin.split('[')[0]}->{pin.split('[')[0]}.rise_transition", 0)
  fall_transition = pred.get(f"{related_pin.split('[')[0]}->{pin.split('[')[0]}.fall_transition", 0)
  if OPS[pred["op"]][1] == "all" and "sh" not in pred["op"]:
    i = int(pin.split("]")[0].split("[")[-1]) if "[" in pin else 0
    rpi = int(related_pin.split("]")[0].split("[")[-1]) if "[" in related_pin else 0
    cell_rise *= (i - rpi + 1) / pred["y_width"]
    cell_fall *= (i - rpi + 1) / pred["y_width"]
  timing.update({
    "cell_rise": { "scalar": { "values": (cell_rise,) } },
    "cell_fall": { "scalar": { "values": (cell_fall,) } },
    "rise_transition": { "scalar": { "values": (rise_transition,) } },
    "fall_transition": { "scalar": { "values": (fall_transition,) } }
  })
  return timing

def pred2timing(pin, direction, ctype, params, pred):
  """Converts prediction to Liberty timing"""
  # Get attributes from cell type
  unate, style, function, related_pins, _, _ = OPS[ctype.replace("_unclk", "")]

  # Deal with flip-flops, latches, memories, and input pins
  if (pin in ["D", "EN"]) and ("ff" in ctype):
    return setup2lib(pin, "CLK", pred, params["CLK_POLARITY"])
  if ctype == "mem_v2":
    if pin in ["RD_EN", "RD_ADDR"]:
      return setup2lib(pin, "RD_CLK", pred, params["RD_CLK_POLARITY"])
    if pin in ["WR_EN", "WR_ADDR", "WR_DATA"]:
      return setup2lib(pin, "WR_CLK", pred, params["WR_CLK_POLARITY"])
    if pin == "RD_DATA":
      return timing2lib(pin, "RD_CLK", pred, None, params["RD_CLK_POLARITY"])
  if ctype == "mem_v2_unclk":
    if pin in ["RD_EN", "RD_ADDR"]:
      return []
    if pin in ["WR_EN", "WR_ADDR", "WR_DATA"]:
      return setup2lib(pin, "WR_CLK", pred, params["WR_CLK_POLARITY"])
    if pin == "RD_DATA":
      timings = [
        timing2lib(pin, "RD_ADDR", pred, "non_unate"),
        timing2lib(pin, "RD_EN", pred, "non_unate")
      ]
      if params["RD_TRANSPARENCY_MASK"]:
        timings.append(timing2lib(pin, "WR_DATA", pred, "positive_unate"))
      return timings
  if (pin == "D") and ("dlatch" in ctype):
    return setup2lib(pin, "EN", pred, 1 - params["EN_POLARITY"])
  if (pin == "Q") and ("dlatch" in ctype):
    return [
      timing2lib(pin, "D", pred, "positive_unate"),
      timing2lib(pin, "EN", pred, "non_unate", params["EN_POLARITY"])
    ]
  if "out" not in direction:
    return []

  # Get related pins for output pins based on style
  if style == "across" and function != "":
    return [
      timing2lib(pin, related_pin, pred, "non_unate" * (pin == "Q"), params.get("CLK_POLARITY"))
      for related_pin in related_pins.split()
    ]
  if style == "all":
    if "sh" in ctype:
      related_pins = [" ".join([
        f"{p}[{i}]" for i in range(params[f"{p}_WIDTH"])
      ]) if params[f"{p}_WIDTH"] > 1 else p for p in related_pins.split()]
    elif "[" in pin:
      i = int(pin.split("]")[0].split("[")[-1])
      related_pins = [
        f"{p}[{j}]" if params[f"{p}_WIDTH"] > 1 else p
        for p in related_pins.split()
        for j in range(min(i+1, params[f"{p}_WIDTH"]))
      ]
  elif style in ["reduce", "reduce_all"]:
    related_pins = [" ".join([
      f"{p}[{i}]" for i in range(params[f"{p}_WIDTH"])
    ]) if params[f"{p}_WIDTH"] > 1 else p for p in related_pins.split()]
  elif style == "pmux":
    related_pins = ["A"] + \
      [" ".join([
        f"S[{i}]" for i in range(params["S_WIDTH"])
      ]) if params["S_WIDTH"] > 1 else "S"] + \
      [" ".join([
        f"B[{i+params['WIDTH']-1}:{i}]"
        for i in range(0, params["WIDTH"] * params["S_WIDTH"], params["WIDTH"])
      ])]
  elif style == "bmux":
    related_pins = [" ".join([
      f"S[{i}]" for i in range(params["S_WIDTH"])
    ]) if params["S_WIDTH"] > 1 else "S"] + [" ".join([
      f"A[{i+params['WIDTH']-1}:{i}]"
      for i in range(0, params["WIDTH"] << params["S_WIDTH"], params["WIDTH"])
    ])]
  else:
    return []

  # Split up very large buses into multiple related pin chunks (max size 64)
  related_pins_unsplit, related_pins = related_pins, []
  for related_pin in related_pins_unsplit:
    related_pin_list = related_pin.split()
    related_pin_chunks = [
      related_pin_list[i * 64:(i + 1) * 64]
      for i in range((len(related_pin_list) + 64 - 1) // 64)
    ]
    related_pins.extend([" ".join(related_pin_chunk) for related_pin_chunk in related_pin_chunks])

  # Return timing
  return [timing2lib(pin, related_pin, pred, unate) for related_pin in related_pins]

def pred2libpin(pin, width, direction, ctype, params, pred, libcell):
  """Converts pin + prediction to Liberty pin"""
  # Get whether the pin/bus is a pin or a bus
  pinbus = "bus" if width > 1 else "pin"

  # Initialize pin/bus
  libcell[pinbus] = libcell[pinbus] if pinbus in libcell else {}
  libcell[pinbus][pin] = {}

  # Set pin/bus capacitance if input
  if "in" in direction:
    # Treat ALOAD, AD, CLK as zero capacitance (one other special case for shifter)
    try:
      if (pred["mode"] == 1) and \
        (((pin == "A") and ("sh" in ctype)) or ((pin == "B") and ("sh" not in ctype))) or \
        (pin == "A" and ctype == "pmux" and "A_cap" not in pred) or \
        (pin == "RD_EN" and not params["RD_CLK_ENABLE"]) or \
        (pin == "WR_EN" and not params["WR_CLK_ENABLE"]) or \
        (pin in ["ALOAD", "AD", "CLK", "RD_CLK", "WR_CLK", "RD_ARST", "RD_SRST"]):
        libcell[pinbus][pin]["capacitance"] = 0
      else:
        libcell[pinbus][pin]["capacitance"] = pred[f"{pin.replace('_', '')}_cap"]
    except KeyError:
      log.error("Failed on: %s %s %s %s %s %s", pin, width, direction, ctype, params, pred)

  # Set pin/bus function if present
  _, style, function, _, _, _ = OPS[ctype.replace("_unclk", "")]
  if "out" in direction:
    if style == "across" and function != "":
      libcell[pinbus][pin]["function"] = function

  # Set pin/bus attributes
  if "CLK" in pin or ((pin == "EN") and ("latch" in ctype)):
    if ctype != "mem_v2_unclk":
      libcell[pinbus][pin]["clock"] = True
  if pinbus == "bus":
    libcell[pinbus][pin]["bus_type"] = f"bus{width}"
  libcell[pinbus][pin]["direction"] = direction

  # Deal with memory attributes
  if "mem" in ctype:
    if pin == "RD_DATA":
      libcell[pinbus][pin]["memory_read"] = {"address": "RD_ADDR"}
      if params["RD_CLK_ENABLE"]:
        libcell[pinbus][pin]["memory_read"]["clocked_on"] = "RD_CLK"
    if pin == "WR_DATA":
      libcell[pinbus][pin]["memory_write"] = {"address": "WR_ADDR"}
      if params["WR_CLK_ENABLE"]:
        libcell[pinbus][pin]["memory_write"]["clocked_on"] = "WR_CLK"
      else:
        libcell[pinbus][pin]["memory_write"]["enable"] = "WR_EN"

  # Set pin/bus timing (delete if empty)
  libcell[pinbus][pin]["timing"] = pred2timing(pin, direction, ctype, params, pred)
  if libcell[pinbus][pin]["timing"] == []:
    del libcell[pinbus][pin]["timing"]

def pred2libcell(cell, pred):
  """Converts cell + prediction to Liberty cell"""
  # Convert things back
  ctype = cell.type.str().strip("$")
  params = {k.str().strip("\\") : v.as_int() for k, v in cell.parameters.items()}

  # Log what is being dumped
  log.info("Dumping prediction for: %s, %s", ctype, params)
  log.debug("Prediction: %s", pred)

  # Create libcell
  libcell = {
    "area": pred["area"]
  }

  # Get FF/latch/memory libcell headers
  if "dff" in ctype:
    libcell["ff" if params["WIDTH"] == 1 else "ff_bank"] = {
      f""""IQ", "IQN"{f", {params['WIDTH']}" if params["WIDTH"] > 1 else ""}""": {
        "next_state": (f"(D & {'' if params['EN_POLARITY'] else '!'}EN) " +
          f"| (IQ & {'!' if params['EN_POLARITY'] else ''}EN)") if ctype[-1] == "e" else "D",
        "clocked_on": "CLK"
      }
    }
  if "latch" in ctype:
    libcell["latch" if params["WIDTH"] == 1 else "latch_bank"] = {
      f""""IQ", "IQN"{f", {params['WIDTH']}" if params["WIDTH"] > 1 else ""}""": {
        "data_in": "D",
        "enable": f'{"" if params["EN_POLARITY"] else "!"}EN'
      }
    }
  if "mem" in ctype:
    libcell["memory"] = {
      "type": "ram" if params["WR_PORTS"] > 0 else "rom",
      "address_width": params["ABITS"],
      "word_width": params["WIDTH"]
    }
    libcell["is_memory"] = True

  # Output cell attributes and pins
  for k, v in sorted(params.items()):
    # Instantiate pins based on width parameters
    if "_WIDTH" in k:
      if OPS[ctype][1] == "all" and k[0] == "Y" and "sh" not in ctype:
        libcell["bus"] = libcell["bus"] if "bus" in libcell else {}
        libcell["bus"]["Y"] = {"bus_type": f"bus{v}", "direction": "output"}
        for i in range(v):
          pred2libpin(f"Y[{i}]", 1, "output", ctype, params, pred, libcell["bus"]["Y"])
      else:
        pred2libpin(k[0], v, "output" if k[0] == "Y" else "input", ctype, params, pred, libcell)
  if ctype == "mux":
    pred2libpin("A", params["WIDTH"], "input", ctype, params, pred, libcell)
    pred2libpin("B", params["WIDTH"], "input", ctype, params, pred, libcell)
    pred2libpin("S", 1, "input", ctype, params, pred, libcell)
    pred2libpin("Y", params["WIDTH"], "output", ctype, params, pred, libcell)
  if ctype == "bmux":
    pred2libpin("A", params["WIDTH"] << params["S_WIDTH"], "input", ctype, params, pred, libcell)
    pred2libpin("Y", params["WIDTH"], "output", ctype, params, pred, libcell)
  if ctype == "pmux":
    pred2libpin("A", params["WIDTH"], "input", ctype, params, pred, libcell)
    pred2libpin("B", params["WIDTH"] * params["S_WIDTH"], "input", ctype, params, pred, libcell)
    pred2libpin("Y", params["WIDTH"], "output", ctype, params, pred, libcell)
  if "dff" in ctype:
    pred2libpin("CLK", 1, "input", ctype, params, pred, libcell)
    pred2libpin("D", params["WIDTH"], "input", ctype, params, pred, libcell)
    pred2libpin("Q", params["WIDTH"], "output", ctype, params, pred, libcell)
    if ctype[0] == "s":
      pred2libpin("SRST", 1, "input", ctype, params, pred, libcell)
    if "al" in ctype:
      pred2libpin("ALOAD", 1, "input", ctype, params, pred, libcell)
      pred2libpin("AD", params["WIDTH"], "input", ctype, params, pred, libcell)
    elif ctype[0] == "a":
      pred2libpin("ARST", 1, "input", ctype, params, pred, libcell)
    if ctype[-1] == "e":
      pred2libpin("EN", 1, "input", ctype, params, pred, libcell)
  if "dlatch" in ctype:
    pred2libpin("D", params["WIDTH"], "input", ctype, params, pred, libcell)
    pred2libpin("EN", 1, "input", ctype, params, pred, libcell)
    pred2libpin("Q", params["WIDTH"], "output", ctype, params, pred, libcell)
    if ctype[0] == "a":
      pred2libpin("ARST", 1, "input", ctype, params, pred, libcell)
  if "sr" in ctype:
    pred2libpin("SET", params["WIDTH"], "input", ctype, params, pred, libcell)
    pred2libpin("CLR", params["WIDTH"], "input", ctype, params, pred, libcell)
  if ctype == "sr":
    pred2libpin("Q", params["WIDTH"], "output", ctype, params, pred, libcell)
  if ctype == "mem_v2":
    if cell.connections()[ys.IdString("\\RD_CLK")].is_fully_undef():
      ctype = "mem_v2_unclk"
    pred2libpin("RD_CLK", params["RD_PORTS"], "input", ctype, params, pred, libcell)
    pred2libpin("RD_EN", params["RD_PORTS"], "input", ctype, params, pred, libcell)
    pred2libpin("RD_ADDR",
                params["RD_PORTS"]*params["ABITS"], "input", ctype, params, pred, libcell)
    pred2libpin("RD_DATA",
                params["RD_PORTS"]*params["WIDTH"], "output", ctype, params, pred, libcell)
    pred2libpin("RD_ARST", params["RD_PORTS"], "input", ctype, params, pred, libcell)
    pred2libpin("RD_SRST", params["RD_PORTS"], "input", ctype, params, pred, libcell)
    pred2libpin("WR_CLK", params["WR_PORTS"], "input", ctype, params, pred, libcell)
    pred2libpin("WR_EN", params["WR_PORTS"]*params["WIDTH"], "input", ctype, params, pred, libcell)
    pred2libpin("WR_ADDR",
                params["WR_PORTS"]*params["ABITS"], "input", ctype, params, pred, libcell)
    pred2libpin("WR_DATA",
                params["WR_PORTS"]*params["WIDTH"], "input", ctype, params, pred, libcell)

  # Return libcell
  return libcell

def liberty2module(design, config):
  """Converts Liberty files to JSON, then create blackbox models for Yosys/Verific elaboration"""
  # Create set of modules
  all_modules = set(design.modules_)

  # For each Liberty file, create a module
  for liberty_file in config["liberty_files"]:

    # Extract leakage power and groups
    log.info("Parsing Liberty file: %s", liberty_file)
    library = liberty2dict(liberty_file).get("library", {})
    leakage_power_unit = library.get("leakage_power_unit", "1pW")

    # Keep track of type widths
    type_widths = {}

    # Group parsing
    for group in library.get("groups", []):
      # Record bus type widths
      if "type" in group:
        type_name = group.get("type", {}).get("names", [])
        if len(type_name):
          type_name = type_name[0]
        else:
          log.warning("No name for type: %s", group.get("type", {}))
          continue
        type_widths[type_name] = group.get("type", {})

      # Create a blackbox module for each Liberty cell
      elif "cell" in group:
        # Get cell name
        cell = group["cell"]
        name = cell.get("names", [])
        if len(name):
          name = name[0]
        else:
          log.warning("No name for cell: %s", cell)
          continue
        mod_name = ys.IdString(f'\\{name}')
        if mod_name in all_modules:
          log.warning("Library cell already defined: %s", name)
          continue
        all_modules.add(mod_name)

        # Get PPA numbers (TODO: add dynamic power data as attributes too)
        area = cell.get("area", 0)
        cell_leakage_power = cell.get("cell_leakage_power", 0)

        # Create module and set attributes for PPA and blackboxing
        log.info("  Creating Liberty module: %s", name)
        module = design.addModule(mod_name)
        module.set_bool_attribute(ys.IdString("\\blackbox"))
        module.set_string_attribute(ys.IdString("\\area"), str(area))
        module.set_string_attribute(ys.IdString("\\LeakagePower"), str(cell_leakage_power))
        module.set_string_attribute(ys.IdString("\\leakage_power_unit"), str(leakage_power_unit))

        # Create ports
        for cellgroup in cell.get("groups", []):
          # Get pins
          if "pin" in cellgroup:
            direction = cellgroup["pin"].get("direction", "input")
            for pin_name in cellgroup["pin"].get("names", []):
              add_port(module, pin_name, 1, direction)
          # Get buses
          elif "bus" in cellgroup:
            # Get size, direction, upto, offset
            direction = cellgroup["bus"].get("direction", "input")
            bus_type = cellgroup["bus"].get("bus_type", "")
            if bus_type in type_widths:
              downto = type_widths[bus_type].get("downto", False)
              bit_from = type_widths[bus_type].get("bit_from", 0)
              bit_to = type_widths[bus_type].get("bit_to", 0)
              size = type_widths[bus_type].get("bit_width", abs(bit_to - bit_from) + 1)
              offset = bit_from if downto else bit_to
              upto = downto # weird naming
            else:
              log.warning("No type found for bus: %s", pin_name)
              continue
            for bus_name in cellgroup["bus"].get("names", []):
              add_port(module, bus_name, size, direction, upto, offset)
          # Get bundles
          elif "bundle" in cellgroup:
            # Add ports for each member
            direction = cellgroup["bundle"].get("direction", "input")
            for pin_name in cellgroup["bundle"].get("members", []):
              add_port(module, pin_name, 1, direction)
          # Skip group if not pin or bus
          else:
            continue

        # Fix up ports
        module.fixup_ports()

def add_port(module, name, size, direction, upto=False, offset=0):
  """Add a port to a module"""
  port = module.addWire(ys.IdString(f'\\{name}'), size)
  port.upto = upto
  port.start_offset = offset
  if "in" in direction:
    port.port_input = 1
  elif "out" in direction:
    port.port_output = 1
  else:
    log.warning("Unknown direction: %s", direction)
  return port
