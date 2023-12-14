def parse_files(file_paths):
    cpu_graphics_records = {}
    gpu_graphics_records = {}

    cpu_compute_records = {}
    gpu_compute_records = {}
    for file_path in file_paths:
        compute_runtime = 0
        graphics_runtime = 0
        with open(file_path, "r") as file:
            lines = file.readlines()

            # Skipping first 39 lines
            lines = lines[39:]

            num_records = 0
            i = 0
            while i < len(lines):
                compute_runtime += float(lines[i].split(" ")[1])
                compute_runtime += float(lines[i + 1].split(" ")[1])
                compute_runtime += float(lines[i + 2].split(" ")[1])
                graphics_runtime += float(lines[i + 3].split(" ")[2])
                graphics_runtime += float(lines[i + 4].split(" ")[2])
                # record_data = lines[i : i + 5]  # Get the next 5 lines
                # records.append(record_data)
                num_records += 1
                i += 6  # Move to the next record

            # all_records.append(records)
            if file_path.split("/")[-1][:3] == "gpu":
                gpu_graphics_records[
                    file_path.split("/")[-1].split("_")[1].split(".")[0]
                ] = (graphics_runtime / num_records)
                gpu_compute_records[
                    file_path.split("/")[-1].split("_")[1].split(".")[0]
                ] = (compute_runtime / num_records)
            else:
                cpu_graphics_records[
                    file_path.split("/")[-1].split("_")[1].split(".")[0]
                ] = (graphics_runtime / num_records)
                cpu_compute_records[
                    file_path.split("/")[-1].split("_")[1].split(".")[0]
                ] = (compute_runtime / num_records)

    return (
        cpu_compute_records,
        cpu_graphics_records,
        gpu_compute_records,
        gpu_graphics_records,
    )


start_file_path = "/home/apal6981/school/bellhop-opengl/evaluation_data/"
# Usage
file_paths = [
    "gpu_100.txt",
    "gpu_200.txt",
    "gpu_300.txt",
    "gpu_400.txt",
    "gpu_500.txt",
    "gpu_600.txt",
    "gpu_700.txt",
    "gpu_800.txt",
    "gpu_900.txt",
    "gpu_1000.txt",
    "cpu_100.txt",
    "cpu_200.txt",
    "cpu_300.txt",
    "cpu_400.txt",
    "cpu_500.txt",
    "cpu_600.txt",
    "cpu_700.txt",
    "cpu_800.txt",
    "cpu_900.txt",
    "cpu_1000.txt",
]  # Add your file paths here
stamps = ["100", "200", "300", "400", "500", "600", "700", "800", "900", "1000"]
parsed_data = parse_files([start_file_path + file for file in file_paths])

print(parsed_data)

import matplotlib.pyplot as plt

plt.figure(figsize=(12, 5))

plt.subplot(1, 2, 1)
plt.plot(
    parsed_data[0].keys(),
    parsed_data[0].values(),
    marker="o",
    linestyle="-",
    color="blue",
)
plt.plot(
    parsed_data[2].keys(),
    parsed_data[2].values(),
    marker="o",
    linestyle="-",
    color="orange",
)
plt.xlabel("Number of rays")
plt.ylabel("Time (ms)")
plt.title("Computation Time")
plt.legend(["CPU", "GPU"])

plt.subplot(1, 2, 2)
plt.plot(
    parsed_data[1].keys(),
    parsed_data[1].values(),
    marker="o",
    linestyle="-",
    color="blue",
)
plt.plot(
    parsed_data[3].keys(),
    parsed_data[3].values(),
    marker="o",
    linestyle="-",
    color="orange",
)
plt.xlabel("Number of rays")
plt.ylabel("Time (ms)")
plt.title("Graphics Time")
plt.legend(["CPU", "GPU"])

plt.tight_layout()
plt.show()
# Displaying parsed data
# for idx, file_data in enumerate(parsed_data):
#     print(f"Data from file {idx + 1}:")
#     for record in file_data:
#         for line in record:
#             print(line.strip())
#         print("---- End of Record ----")
#     print("#####################################")

print("Speed up values")
print("Computation")
for stamp in stamps:
    print(f"Stamp: {stamp}, {parsed_data[0][stamp]/parsed_data[2][stamp]}")

print("Graphics")
for stamp in stamps:
    print(f"Stamp: {stamp}, {parsed_data[1][stamp]/parsed_data[3][stamp]}")
