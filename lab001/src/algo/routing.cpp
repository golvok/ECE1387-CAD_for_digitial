#include "routing.hpp"

#include <algo/maze_router.hpp>
#include <device/connectors.hpp>
#include <device/device.hpp>
#include <graphics/graphics_wrapper.hpp>
#include <util/logging.hpp>

namespace algo {

template<typename Netlist, typename FanoutGenerator>
std::vector<std::vector<device::RouteElementID>> route_all(const Netlist& pin_to_pin_netlist, FanoutGenerator&& fanout_gen) {
	std::vector<std::vector<device::RouteElementID>> result;
	std::unordered_set<device::RouteElementID> used;

	for (const auto& src_sinks : pin_to_pin_netlist) {
		const auto src_pin_re = device::RouteElementID(src_sinks.first);
		for (const auto& sink_pin : src_sinks.second) {
			const auto sink_pin_re = device::RouteElementID(sink_pin);

			auto indent = dout(DL::INFO).indentWithTitle([&](auto&& str) {
				str << "Routing " << src_pin_re << " -> " << sink_pin_re;
			});

			result.emplace_back(algo::maze_route<device::RouteElementID>(src_pin_re, sink_pin_re, fanout_gen, [&](auto&& reid) {
				return used.find(reid) != end(used);
			}));

			const auto& path = result.back();

			for (const auto& id : path) {
				if (id != src_pin_re) {
					used.insert(id);
				}
			}

			const auto gfx_state_keeper = graphics::get().fpga().pushState(&fanout_gen, {path});
			graphics::get().waitForPress();
		}
	}
	return result;
}

template std::vector<std::vector<device::RouteElementID>> route_all(const util::Netlist<device::PinGID>& pin_to_pin_netlist,       device::Device<device::FullyConnectedConnector>&  fanout_gen);
template std::vector<std::vector<device::RouteElementID>> route_all(const util::Netlist<device::PinGID>& pin_to_pin_netlist, const device::Device<device::FullyConnectedConnector>&  fanout_gen);
template std::vector<std::vector<device::RouteElementID>> route_all(const util::Netlist<device::PinGID>& pin_to_pin_netlist,       device::Device<device::FullyConnectedConnector>&& fanout_gen);

} // end namespace algo
