import os
import matplotlib.pyplot as plt

# Function to parse the metrics file and calculate means
def parse_metrics_file(filename):
    # Dictionary to store the mean values for each case
    means = {}
    sizes = {}
    energies = {}

    # Open the file and read lines
    with open(filename, 'r') as file:
        lines = file.readlines()

    # Parse lines and calculate means
    for line in lines:
        line = line.strip()
        if line.startswith("Text to divisor:") or line.startswith("Text to EC Point:"):
            case = "Encoding"
            time_value = int(line.split(":")[1].split(' ')[1].strip()) / 1000  # Convert microseconds to milliseconds

            if case in means:
                means[case].append(time_value)
            else:
                means[case] = [time_value]

        elif line.startswith("Divisor to text:") or line.startswith("EC Point to Text:"):
            case = "Decoding"
            time_value = int(line.split(":")[1].split(' ')[1].strip()) / 1000  # Convert microseconds to milliseconds

            if case in means:
                means[case].append(time_value)
            else:
                means[case] = [time_value]

        elif line.startswith("Certificate generation:") or \
           line.startswith("Certificate public key extraction:") or \
           line.startswith("Certificate private key reception:") or \
           line.startswith("Message encryption:") or \
           line.startswith("Message decryption:") or \
           line.startswith("Signature generation:") or \
           line.startswith("Signature verification:"):

            case, time_value = line.split(":")
            time_value = int(time_value.split(' ')[1].strip()) / 1000  # Convert microseconds to milliseconds

            if case in means:
                means[case].append(time_value)
            else:
                means[case] = [time_value]

        elif line.startswith("Public key generation:"):
            case = "Key-Pair generation time"
            time_value = int(line.split(":")[1].split(' ')[1].strip()) / 1000  # Convert microseconds to milliseconds

            if case in means:
                means[case].append(time_value)
            else:
                means[case] = [time_value]
        
        elif line.startswith("RSU_CERT_BROADCAST message size:") or \
                line.startswith("VEHICLE_SEND_JOIN_RSU message size:") or \
                line.startswith("RSU_ACCEPT message size:") or \
                line.startswith("RSU_INFORM_LEADER message size:") or \
                line.startswith("GL_LEADERSHIP_PROOF message size:") or \
                line.startswith("VEHICLE_INFORM message size:") or \
                line.startswith("VEHICLE_SEND_JOIN_GL message size:") or \
                line.startswith("GL_ACCEPT message size:"):
            
            case, size_value = line.split(":")
            size_value = int(size_value.split(' ')[1].strip())

            if case in sizes:
                continue
            else:
                sizes[case] = size_value

        elif line.startswith("RECEIVE_CERT consumption:") or \
           line.startswith("RECEIVE_ACCEPT_SYMMETRIC consumption:") or \
           line.startswith("EXTRACT_JOIN_GL_SEND_ACCEPT consumption:") or \
           line.startswith("EXTRACT_GL_PROOF consumption:") or \
           line.startswith("EXTRACT_INFO_GL consumption:"):

            case, energy_value = line.split(":")
            energy_value = float(energy_value.split(' ')[1].strip())
            

            if case in energies:
                energies[case].append(energy_value)
            else:
                energies[case] = [energy_value]

    # Calculate means for each case
    for case, values in means.items():
        means[case] = sum(values) / len(values)

    for case, values in energies.items():
        energies[case] = sum(values) / len(values)
    return means, sizes, energies


# Create the metrics_plots directory if it doesn't exist
output_dir = "metrics_plots"
os.makedirs(output_dir, exist_ok=True)

# Parse metrics_g2.txt and calculate means
metrics_file_g2 = 'metrics_g2.txt'
means_g2, sizes_g2, energies_g2 = parse_metrics_file(metrics_file_g2)

# Parse metrics_g3.txt and calculate means
metrics_file_g3 = 'metrics_g3.txt'
means_g3, sizes_g3, energies_g3 = parse_metrics_file(metrics_file_g3)

# Parse metrics_ec.txt and calculate means
metrics_file_ec = 'metrics_ec.txt'
means_ec, sizes_ec, energies_ec = parse_metrics_file(metrics_file_ec)

# Plot and save bar plots for times
for case, mean_value_g2 in means_g2.items():
    mean_value_g3 = means_g3.get(case, 0)
    mean_value_ec = means_ec.get(case, 0)

    plt.figure(figsize=(8, 6))
    plt.bar('EC', mean_value_ec, color='green', width=0.2)
    plt.bar('HEC genus 2', mean_value_g2, color='blue', width=0.2)
    plt.bar('HEC genus 3', mean_value_g3, color='orange', width=0.2)
    

    plt.ylabel('Time (ms)')
    plt.title(case.replace('Text to divisor', 'Encoding')
                    .replace('Divisor to text', 'Decoding')
                    .replace('Text to EC Point', 'Encoding')
                    .replace('EC Point to Text', 'Decoding'))
    plt.ylim(top=max(mean_value_g2, mean_value_g3, mean_value_ec) * 1.5)
    plt.grid(axis='y', linestyle='-', linewidth=0.5, alpha=0.3, zorder=0)
    plt.savefig(os.path.join(output_dir, f"{case}.png"))
    plt.close()

    # Print mean values
    print(f"{case}: HEC genus 2: {mean_value_g2:.2f} ms, HEC genus 3: {mean_value_g3:.2f} ms, EC: {mean_value_ec:.2f} ms")

# Plot bar plots for sizes
for case, size_values_g2 in sizes_g2.items():
    size_values_g3 = sizes_g3.get(case, 0)
    size_values_ec = sizes_ec.get(case, 0)

    plt.figure(figsize=(8, 6))
    plt.bar('EC', size_values_ec, color='green', width=0.2)
    plt.bar('HEC genus 2', size_values_g2, color='blue', width=0.2)
    plt.bar('HEC genus 3', size_values_g3, color='orange', width=0.2)
    

    plt.ylabel('Size (bytes)')
    plt.title(case)
    plt.ylim(top=max(size_values_g2, size_values_g3, size_values_ec) * 1.5)
    plt.grid(axis='y', linestyle='-', linewidth=0.5, alpha=0.3, zorder=0)
    
    plt.savefig(os.path.join(output_dir, f"{case}_sizes.png"))
    plt.close()

    # Print sizes
    print(f"{case}: HEC genus 2: {size_values_g2}, HEC genus 3: {size_values_g3}, EC: {size_values_ec}")


# Plot and save bar plots for energies of whole exchanges
plt.figure(figsize=(20, 15))
cases = list(case for case in energies_g2.keys() if (case != "RECEIVE_CERT consumption" and case != "EXTRACT_GL_PROOF consumption"))  # Get the list of cases

# Set the x-axis positions for the bars
x_positions1 = list(range(len(cases)))
x_positions = [0.8*x for x in x_positions1]

# Set the width of each bar
bar_width = 0.15

# Plot the bars for EC
plt.bar([x - bar_width for x in x_positions], [val for key, val in energies_ec.items() if (key != "RECEIVE_CERT consumption" and key != "EXTRACT_GL_PROOF consumption")], width=bar_width, color='green', alpha=0.8, label='EC')

# Plot the bars for HEC genus 2
plt.bar(x_positions, [val for key, val in energies_g2.items() if (key != "RECEIVE_CERT consumption" and key != "EXTRACT_GL_PROOF consumption")], width=bar_width, color='blue', alpha=0.8, label='HEC genus 2')

# Plot the bars for HEC genus 3
plt.bar([x + bar_width for x in x_positions], [val for key, val in energies_g3.items() if (key != "RECEIVE_CERT consumption" and key != "EXTRACT_GL_PROOF consumption")], width=bar_width, color='orange', alpha=0.8, label='HEC genus 3')

# Set the x-axis labels
plt.xticks(x_positions, cases, rotation=45, fontsize=18)

# Set the y-axis label
plt.ylabel('Energy (J)', fontsize=18)
plt.ylim(top=max(max(energies_ec.values()), max(energies_g2.values()), max(energies_g3.values())) * 1.5)
plt.yticks(fontsize=18)

# Set the title
plt.title('Energy Consumption', fontsize=18)

# Add legend
plt.legend(fontsize=18)
plt.tight_layout()
plt.grid(axis='y', linestyle='-', linewidth=0.5, alpha=0.5)

# Save the plot
plt.savefig(os.path.join(output_dir, "Energy_consumption1.png"))
plt.close()


#Plot energies of simple cert reception
plt.figure(figsize=(20, 15))
cases = list(case for case in energies_g2.keys() if (case == "RECEIVE_CERT consumption" or case == "EXTRACT_GL_PROOF consumption"))  # Get the list of cases

# Set the x-axis positions for the bars
x_positions1 = list(range(len(cases)))
x_positions = [0.8*x for x in x_positions1]

# Set the width of each bar
bar_width = 0.1

# Plot the bars for EC
plt.bar([x - bar_width for x in x_positions], [val for key, val in energies_ec.items() if (key == "RECEIVE_CERT consumption" or key == "EXTRACT_GL_PROOF consumption")], width=bar_width, color='green', alpha=0.8, label='EC')

# Plot the bars for HEC genus 2
plt.bar(x_positions, [val for key, val in energies_g2.items() if (key == "RECEIVE_CERT consumption" or key == "EXTRACT_GL_PROOF consumption")], width=bar_width, color='blue', alpha=0.8, label='HEC genus 2')

# Plot the bars for HEC genus 3
plt.bar([x + bar_width for x in x_positions], [val for key, val in energies_g3.items() if (key == "RECEIVE_CERT consumption" or key == "EXTRACT_GL_PROOF consumption")], width=bar_width, color='orange', alpha=0.8, label='HEC genus 3')

# Set the x-axis labels
plt.xticks(x_positions, cases, rotation=45, fontsize=18)

# Set the y-axis label
plt.ylabel('Energy (J)', fontsize=18)
plt.ylim(top=max(max(energies_ec["RECEIVE_CERT consumption"], energies_ec["EXTRACT_GL_PROOF consumption"]), max(energies_g2["RECEIVE_CERT consumption"], energies_g2["EXTRACT_GL_PROOF consumption"]), max(energies_g3["RECEIVE_CERT consumption"], energies_g3["EXTRACT_GL_PROOF consumption"])) * 1.5)
plt.yticks(fontsize=18)

# Set the title
plt.title('Energy Consumption', fontsize=18)

# Add legend
plt.legend(fontsize=18)
plt.tight_layout()
plt.grid(axis='y', linestyle='-', linewidth=0.5, alpha=0.5)

# Save the plot
plt.savefig(os.path.join(output_dir, "Energy_consumption2.png"))
plt.close()

for case, energy_val_ec in energies_ec.items():
    energy_val_g2 = energies_g2.get(case, 0)
    energy_val_g3 = energies_g3.get(case, 0)
    print(f"{case}: HEC genus 2: {energy_val_g2:.5f} J, HEC genus 3: {energy_val_g3:.5f} J, EC: {energy_val_ec:.5f} J")

