#include <iostream>
#include <unordered_map>
#define PAGE_SIZE 4096


struct PhysicalAddress
{
	bool valid;
	uint32_t address;
};

std::unordered_map<uint32_t, PhysicalAddress> pageTable;


void initialize_page_table();
uint32_t translate_virtual_to_physical(uint32_t virtual_address);
void map_virtual_page_to_physical_frame(uint32_t virtual_page, uint32_t physical_frame);
void unmap_virtual_page(uint32_t virtual_page);
void print_page_table();
uint32_t get_page_number(uint32_t virtual_address);
uint32_t get_offset(uint32_t virtual_address);

int main() {
	initialize_page_table();
	print_page_table();

	uint32_t virtual_address = 0x12345;
	uint32_t physical_address = translate_virtual_to_physical(virtual_address);
	std::cout << "Physical Address for " << std::hex << virtual_address << " is: "
		<< std::hex << physical_address << std::endl;

	return 0;
}

void initialize_page_table() {
	pageTable[0x12345] = { true,0x56785 };
	pageTable[0x12346] = { true,0x56786 };
	pageTable[0x12347] = { true,0x56787 };
	pageTable[0x12348] = { true,0x56788 };
	pageTable[0x12349] = { false,0x56789 };
	pageTable[0x1234A] = { false,0x5678A };
	pageTable[0x1234B] = { false,0x5678B };
	pageTable[0x1234C] = { false,0x5678C };

};

uint32_t translate_virtual_to_physical(uint32_t virtual_address) {
	uint32_t virtual_page = get_page_number(virtual_address);
	uint32_t offset = get_offset(virtual_address);

	auto search = pageTable.find(virtual_page);
	if (search != pageTable.end() && search->second.valid) {
		return search->second.address * PAGE_SIZE + offset;
	}

	return 0xFFFFFFFF;
}

void map_virtual_page_to_physical_frame(uint32_t virtual_page, uint32_t physical_frame) {
	pageTable[virtual_page] = { true,physical_frame };
}

void unmap_virtual_page(uint32_t virtual_page) {
	auto search = pageTable.find(virtual_page);
	if (search != pageTable.end() && search->second.valid) {
		search->second.valid = false;
	}
}

void print_page_table() {
	std::cout << "Virtual Page" << "\t" << "Physical Frame" << "\t" << "Valid" << std::endl;
	std::cout << "======================================" << std::endl;
	for (const auto& entry : pageTable) {
		std::cout << std::hex << entry.first << "\t"   // Printing virtual page address in hex
			<< std::hex << entry.second.address << "\t"  // Printing physical frame address in hex
			<< std::boolalpha << entry.second.valid << std::endl;
	}
	std::cout << "======================================" << std::endl;
}


uint32_t get_page_number(uint32_t virtual_address) {
	return virtual_address / PAGE_SIZE;
}
uint32_t get_offset(uint32_t virtual_address) {
	return	virtual_address % PAGE_SIZE;
}

