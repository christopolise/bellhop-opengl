def parse_files(file_paths):
    cpu_graphics_records = {}
    gpu_graphics_records = {}

    cpu_compute_records = {}
    gpu_compute_records = {}
    for file_path in file_paths:
        preprocess_ct = 0
        run_ct = 0
        post_p_ct = 0
        vert_gt = 0
        swap_gt = 0
        total_gt = 0
        with open(file_path, "r") as file:
            lines = file.readlines()

            # Skipping first 43 lines
            lines = lines[43:]

            num_records = 0
            i = 0
            while i < len(lines):
                preprocess_ct += float(lines[i].split(" ")[1])
                run_ct += float(lines[i + 1].split(" ")[1])
                post_p_ct += float(lines[i + 2].split(" ")[1])
                total_gt += float(lines[i + 3].split(" ")[2])
                vert_gt += float(lines[i + 4].split("\t")[1])
                vert_gt += float(lines[i + 5].split("\t")[1])
                swap_gt += float(lines[i + 6].split("\t")[1])
                total_gt += float(lines[i + 7].split(" ")[2])
                # record_data = lines[i : i + 5]  # Get the next 5 lines
                # records.append(record_data)
                num_records += 1
                i += 9  # Move to the next record

    return (
        preprocess_ct / num_records,
        run_ct / num_records,
        post_p_ct / num_records,
        total_gt / num_records,
        vert_gt / num_records,
        swap_gt / num_records,
    )

    # all_records.append(records)
    # if file_path.split("/")[-1][:3] == "gpu":
    #             gpu_graphics_records[
    #                 file_path.split("/")[-1].split("_")[1].split(".")[0]
    #             ] = (graphics_runtime / num_records)
    #             gpu_compute_records[
    #                 file_path.split("/")[-1].split("_")[1].split(".")[0]
    #             ] = (compute_runtime / num_records)
    #         else:
    #             cpu_graphics_records[
    #                 file_path.split("/")[-1].split("_")[1].split(".")[0]
    #             ] = (graphics_runtime / num_records)
    #             cpu_compute_records[
    #                 file_path.split("/")[-1].split("_")[1].split(".")[0]
    #             ] = (compute_runtime / num_records)

    # return (
    #     cpu_compute_records,
    #     cpu_graphics_records,
    #     gpu_compute_records,
    #     gpu_graphics_records,
    # )


start_file_path = "/home/apal6981/school/bellhop-opengl/evaluation_data/"
# Usage
file_paths = [
    "gl_cpu_1000.txt",
    "gl_gpu_1000.txt",
]  # Add your file paths here
# stamps = ["100", "200", "300", "400", "500", "600", "700", "800", "900", "1000"]
parsed_data = parse_files([start_file_path + file_paths[0]])
parsed_data_gpu = parse_files([start_file_path + file_paths[1]])

print(parsed_data)
print(parsed_data_gpu)

import matplotlib.pyplot as plt
import numpy as np

# Sample data for two sets of pie charts
labels1 = [
    "Preprocess",
    "Run",
    "Postprocess",
    "Vertex Population",
    "Swap Buffers",
    "Draw + Other",
]
sizes1 = [
    parsed_data[0],
    parsed_data[1],
    parsed_data[2],
    parsed_data[4],
    parsed_data[5],
    parsed_data[3] - parsed_data[4] - parsed_data[5],
]

labels2 = [
    "Preprocess",
    "Run",
    "Postprocess",
    "Vertex Population",
    "Swap Buffers",
    "Draw + Other",
]
sizes2 = [
    parsed_data_gpu[0],
    parsed_data_gpu[1],
    parsed_data_gpu[2],
    parsed_data_gpu[4],
    parsed_data_gpu[5],
    parsed_data_gpu[3] - parsed_data_gpu[4] - parsed_data_gpu[5],
]

# Creating subplots for two pie charts side by side
fig, axs = plt.subplots(1, 2, figsize=(12, 6))  # 1 row, 2 columns

# First pie chart
wedges1, _ = axs[0].pie(
    sizes1, startangle=140, textprops={"rotation": 45, "ha": "left"}
)
axs[0].set_title("CPU Frame Breakdown")  # Title for the first chart

# Second pie chart
wedges2, _ = axs[1].pie(
    sizes2, startangle=140, textprops={"rotation": 45, "ha": "left"}
)
axs[1].set_title("GPU Frame Breakdown")  # Title for the second chart

# Equal aspect ratio ensures that pie is drawn as a circle
axs[0].axis("equal")
axs[1].axis("equal")

# Create a dummy plot and hide it outside the visible area
axs[0].plot(0, 0, label="Legend Label", alpha=0)  # Dummy invisible plot for legend
axs[1].plot(0, 0, label="Legend Label", alpha=0)  # Dummy invisible plot for legend

# Create legend for the dummy plot (use loc="center left" or other suitable location)
plt.legend(
    [
        "Preprocess",
        "Run",
        "Postprocess",
        "Vertex Population",
        "Swap Buffers",
        "Draw + Other",
    ],
    loc="center left",
    bbox_to_anchor=(1, 0.5),
    title="Legend Title",
)  # Adjust the location/title as needed

# Adjust layout to prevent overlapping titles
plt.tight_layout()

# Show the pie charts
plt.show()