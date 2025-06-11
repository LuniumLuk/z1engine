#include "pch.h"
#include "core/layer_stack.h"

namespace z1 {

	LayerStack::LayerStack() {
		m_layer_insert_index = 0;
	}

	LayerStack::~LayerStack() {
		for (auto const& layer : m_layers) {
			layer->on_detach();
		}
		m_layers.clear();
	}

	void LayerStack::push_layer(std::shared_ptr<Layer> const& layer) {
		m_layers.emplace(m_layers.begin() + m_layer_insert_index, layer);
		m_layer_insert_index++;
		layer->on_attach();
	}

	void LayerStack::push_overlay(std::shared_ptr<Layer> const& overlay) {
		m_layers.emplace_back(overlay);
		overlay->on_attach();
	}

	void LayerStack::pop_layer(std::shared_ptr<Layer> const& layer) {
		auto it = std::find(m_layers.begin(), m_layers.end(), layer);
		if (it != m_layers.end()) {
			m_layers.erase(it);
			layer->on_detach();
			m_layer_insert_index--;
		}
	}

	void LayerStack::pop_overlay(std::shared_ptr<Layer> const& overlay) {
		auto it = std::find(m_layers.begin(), m_layers.end(), overlay);
		if (it != m_layers.end()) {
			m_layers.erase(it);
			overlay->on_detach();
		}
	}

}
