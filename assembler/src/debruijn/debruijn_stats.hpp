/*
 * debruijn_stats.hpp
 *
 *  Created on: Aug 12, 2011
 *      Author: sergey
 */

#ifndef DEBRUIJN_STATS_HPP_
#define DEBRUIJN_STATS_HPP_

#include "visualization_utils.hpp"
#include "statistics.hpp"
#include "new_debruijn.hpp"
#include "edges_position_handler.hpp"
#include "EdgeVertexFilter.hpp"
//#include <boost/filesystem.hpp>
#include "read/osequencestream.hpp"
#include <sys/types.h>
#include <sys/stat.h>

namespace debruijn_graph {

template<class Graph, size_t k>
class GenomeMappingStat: public omnigraph::AbstractStatCounter {
private:
	typedef typename Graph::EdgeId EdgeId;
	Graph &graph_;
	const EdgeIndex<k + 1, Graph>& index_;
	Sequence genome_;
public:
	GenomeMappingStat(Graph &graph, const EdgeIndex<k + 1, Graph> &index,
	Sequence genome) :
			graph_(graph), index_(index), genome_(genome) {
	}

	virtual ~GenomeMappingStat() {
	}

	virtual void Count() {
		INFO("Mapping genome");
		size_t break_number = 0;
		size_t covered_kp1mers = 0;
		size_t fail = 0;
		Seq<k + 1> cur = genome_.start<k + 1>() >> 0;
		bool breaked = true;
		pair<EdgeId, size_t> cur_position;
		for (size_t cur_nucl = k; cur_nucl < genome_.size(); cur_nucl++) {
			cur = cur << genome_[cur_nucl];
			if (index_.containsInIndex(cur)) {
				pair<EdgeId, size_t> next = index_.get(cur);
				if (!breaked
						&& cur_position.second + 1
								< graph_.length(cur_position.first)) {
					if (next.first != cur_position.first
							|| cur_position.second + 1 != next.second) {
						fail++;
					}
				}
				cur_position = next;
				covered_kp1mers++;
				breaked = false;
			} else {
				if (!breaked) {
					breaked = true;
					break_number++;
				}
			}
		}INFO("Genome mapped");
		INFO("Genome mapping results:");
		INFO(
				"Covered k+1-mers:" << covered_kp1mers << " of " << (genome_.size() - k) << " which is " << (100.0 * covered_kp1mers / (genome_.size() - k)) << "%");
		INFO(
				"Covered k+1-mers form " << break_number + 1 << " contigious parts");
		INFO("Continuity failtures " << fail);
	}
};

template<class Graph, size_t k>
class StatCounter: public omnigraph::AbstractStatCounter {
private:
	omnigraph::StatList stats_;
public:
	typedef typename Graph::VertexId VertexId;
	typedef typename Graph::EdgeId EdgeId;

	StatCounter(Graph& graph, const EdgeIndex<k + 1, Graph>& index,
	const Sequence& genome) {
		SimpleSequenceMapper<k + 1, Graph> sequence_mapper(graph, index);
		Path<EdgeId> path1 = sequence_mapper.MapSequence(Sequence(genome));
		Path<EdgeId> path2 = sequence_mapper.MapSequence(!Sequence(genome));
		stats_.AddStat(new omnigraph::VertexEdgeStat<Graph>(graph));
		stats_.AddStat(
				new omnigraph::BlackEdgesStat<Graph>(graph, path1, path2));
		stats_.AddStat(new omnigraph::NStat<Graph>(graph, path1, 50));
		stats_.AddStat(new omnigraph::SelfComplementStat<Graph>(graph));
		stats_.AddStat(
				new GenomeMappingStat<Graph, k>(graph, index,
						Sequence(genome)));
	}

	virtual ~StatCounter() {
		stats_.DeleteStats();
	}

	virtual void Count() {
		stats_.Count();
	}

private:
	DECL_LOGGER("StatCounter")
};

template<size_t k, class Graph>
void CountStats(Graph& g, const EdgeIndex<k + 1, Graph>& index,
		const Sequence& genome) {
	INFO("Counting stats");
	StatCounter<Graph, k> stat(g, index, genome);
	stat.Count();
	INFO("Stats counted");
}

void CountPairedInfoStats(Graph &g, size_t insert_size, size_t max_read_length,
		PairedInfoIndex<Graph> &paired_index,
		PairedInfoIndex<Graph> &etalon_paired_index,
		const string &output_folder) {
	INFO("Counting paired info stats");
	EdgePairStat<Graph> (g, paired_index, output_folder).Count();

	//todo remove filtration if launch on etalon info is ok
	UniquePathStat<Graph> (g, etalon_paired_index, insert_size,
			max_read_length, 0.1 * insert_size, 40.0).Count();
	UniqueDistanceStat<Graph> (etalon_paired_index).Count();
	INFO("Paired info stats counted");
}

void CountClusteredPairedInfoStats(Graph &g, size_t insert_size,
		size_t max_read_length, PairedInfoIndex<Graph> &paired_index,
		PairedInfoIndex<Graph> &clustered_index,
		PairedInfoIndex<Graph> &etalon_paired_index,
		const string &output_folder) {
	INFO("Counting paired info stats");
	EstimationQualityStat<Graph> estimation_stat(g, paired_index,
			clustered_index, etalon_paired_index);
	estimation_stat.Count();
	string stat_folder = output_folder + "/pair_inf_stat";
//	boost::filesystem::create_directory(stat_folder.c_str());
	mkdir(stat_folder.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH | S_IWOTH);
	estimation_stat.WriteEstmationStats(stat_folder);
	ClusterStat<Graph> (clustered_index).Count();
	INFO("Paired info stats counted");
}

void WriteToDotFile(Graph &g, const string& file_name, string graph_name,
		Path<EdgeId> path1/* = Path<EdgeId> ()*/, Path<EdgeId> path2/* = Path<EdgeId> ()*/) {
	INFO("Writing graph '" << graph_name << "' to file " << file_name);
	omnigraph::WritePaired(file_name, graph_name, g, path1, path2);
	INFO("Graph '" << graph_name << "' written to file " << file_name);
}

void DetailedWriteToDot(Graph &g, const string& file_name, string graph_name,
		Path<EdgeId> path1/* = Path<EdgeId> ()*/, Path<EdgeId> path2/* = Path<EdgeId> ()*/) {
	INFO("Writing graph '" << graph_name << "' to file " << file_name);
	omnigraph::WriteToFile(file_name, graph_name, g, path1, path2);
	INFO("Graph '" << graph_name << "' written to file " << file_name);
}

template<size_t k>
Path<typename Graph::EdgeId> FindGenomePath(const Sequence& genome,
		const Graph& g, const EdgeIndex<k + 1, Graph>& index) {
	SimpleSequenceMapper<k + 1, Graph> srt(g, index);
	return srt.MapSequence(genome);
}

template<size_t k>
void ProduceInfo(Graph& g, const EdgeIndex<k + 1, Graph>& index,
		const Sequence& genome, const string& file_name,
		const string& graph_name) {
	CountStats<k> (g, index, genome);
	Path<typename Graph::EdgeId> path1 = FindGenomePath<k> (genome, g, index);
	Path<typename Graph::EdgeId> path2 = FindGenomePath<k> (!genome, g, index);
	WriteToDotFile(g, file_name, graph_name, path1, path2);
}


template<size_t k>
void ProduceNonconjugateInfo(NCGraph& g,
		const EdgeIndex<k + 1, NCGraph>& index, const string& genome,
		const string& work_tmp_dir, const string& graph_name,
		const IdTrackHandler<NCGraph> &IdTrackLabelerResolved) {
	CountStats<k> (g, index, genome);
	//	omnigraph::WriteSimple( file_name, graph_name, g, IdTrackLabelerResolved);
	//	omnigraph::WriteSimple( work_tmp_dir, graph_name, g, IdTrackLabelerResolved);

}

template<size_t k>
void ProduceDetailedInfo(Graph& g, const EdgeIndex<k + 1, Graph>& index,
		const Sequence& genome, const string& folder, const string& file_name,
		const string& graph_name) {
	CountStats<k> (g, index, genome);
	Path<typename Graph::EdgeId> path1 = FindGenomePath<k> (genome, g, index);
	Path<typename Graph::EdgeId> path2 = FindGenomePath<k> (!genome, g, index);

	mkdir(folder.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH | S_IWOTH);
	DetailedWriteToDot(g, folder + file_name, graph_name, path1, path2);
}

template<size_t k>
void WriteGraphComponents(Graph& g, const EdgeIndex<k + 1, Graph>& index,
		const Sequence& genome, const string& folder, const string &file_name,
		const string &graph_name, size_t split_edge_length) {
	Path<typename Graph::EdgeId> path1 = FindGenomePath<k> (genome, g, index);
	Path<typename Graph::EdgeId> path2 = FindGenomePath<k> (!genome, g, index);
	mkdir(folder.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH | S_IWOTH);
	WriteComponents(folder + file_name, graph_name, g, split_edge_length,
			path1, path2);

}

string ConstructComponentName(string file_name, size_t cnt) {
	stringstream ss;
	ss << cnt;
	string res = file_name;
	res.insert(res.length(), ss.str());
	return res;
}

template<class Graph>
int PrintGraphComponents(const string& file_name, Graph& g,
		size_t split_edge_length, IdTrackHandler<Graph> &old_IDs,
		PairedInfoIndex<Graph>  &paired_index,
		EdgesPositionHandler<Graph> &edges_positions) {
	LongEdgesSplitter<Graph> inner_splitter(g, split_edge_length);
	ComponentSizeFilter<Graph> checker(g, split_edge_length);
	FilteringSplitterWrapper<Graph> splitter(inner_splitter, checker);
	size_t cnt = 1;
	while (!splitter.Finished() && cnt <= 1000) {
		string component_name = ConstructComponentName(file_name, cnt).c_str();
		auto component = splitter.NextComponent();
		EdgeVertexFilter<Graph> filter(g, component);
		printGraph(g, old_IDs, component_name, paired_index, edges_positions,
				&filter);
		cnt++;
	}
	return (cnt - 1);

}

template<class Graph>
void OutputContigs(Graph& g, const string& contigs_output_filename) {
	INFO("-----------------------------------------");
	INFO("Outputting contigs to " << contigs_output_filename);
	osequencestream oss(contigs_output_filename);
	for (auto it = g.SmartEdgeBegin(); !it.IsEnd(); ++it) {
		oss << g.EdgeNucls(*it);
	}INFO("Contigs written");
}

template<class Graph>
void OutputSingleFileContigs(Graph& g, const string& contigs_output_dir) {
	INFO("-----------------------------------------");
	INFO("Outputting contigs to " << contigs_output_dir);
	int n = 0;
	mkdir(contigs_output_dir.c_str(),
			S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH | S_IWOTH);
	char n_str[20];
	for (auto it = g.SmartEdgeBegin(); !it.IsEnd(); ++it) {
		sprintf(n_str, "%d.fa", n);

		osequencestream oss(contigs_output_dir + n_str);

		//		osequencestream oss(contigs_output_dir + "tst.fasta");
		oss << g.EdgeNucls(*it);
		n++;
	}INFO("Contigs written");
}


template<size_t k>
void FillEdgesPos(Graph& g, const EdgeIndex<k + 1, Graph>& index,
		const Sequence& genome, EdgesPositionHandler<Graph>& edgesPos) {
	Path<typename Graph::EdgeId> path1 = FindGenomePath<k> (genome, g, index);
	int CurPos = 0;
	for (auto it = path1.sequence().begin(); it != path1.sequence().end(); ++it) {
		EdgeId ei = *it;
		edgesPos.AddEdgePosition(ei, CurPos + 1, CurPos + g.length(ei));
		CurPos += g.length(ei);
	}
	//CurPos = 1000000000;
	CurPos = 0;
	Path<typename Graph::EdgeId> path2 = FindGenomePath<k> (!genome, g, index);
	for (auto it = path2.sequence().begin(); it != path2.sequence().end(); ++it) {
		CurPos -= g.length(*it);
	}
	for (auto it = path2.sequence().begin(); it != path2.sequence().end(); ++it) {
		EdgeId ei = *it;
		edgesPos.AddEdgePosition(ei, CurPos, CurPos + g.length(ei)-1);
		CurPos += g.length(ei);
	}
}


}

#endif /* DEBRUIJN_STATS_HPP_ */
