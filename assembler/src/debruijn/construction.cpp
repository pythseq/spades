//***************************************************************************
//* Copyright (c) 2011-2013 Saint-Petersburg Academic University
//* All Rights Reserved
//* See file LICENSE for details.
//****************************************************************************

#include "standard.hpp"
#include "io/easy_reader.hpp"
#include "io/vector_reader.hpp"
#include "omni_labelers.hpp"
#include "dataset_readers.hpp"
#include "graph_pack.hpp"
#include "read_converter.hpp"

#include "graph_construction.hpp"
#include "debruijn_stats.hpp"
#include "construction.hpp"

namespace debruijn_graph {

template<class Read>
void construct_graph(io::ReadStreamVector< io::IReader<Read> >& streams,
                     conj_graph_pack& gp, ReadStream* contigs_stream = 0) {
    debruijn_config::construction params = cfg::get().con;
    params.early_tc.enable &= !cfg::get().ds.single_cell;

    size_t rl = ConstructGraphWithCoverage(cfg::get().K, params, streams, gp.g,
                                           gp.index, gp.flanking_cov, contigs_stream);

    if (!cfg::get().ds.RL()) {
        INFO("Figured out: read length = " << rl);
        cfg::get_writable().ds.set_RL(rl);
    } else if (cfg::get().ds.RL() != rl)
        WARN("In datasets.info, wrong RL is specified: " << cfg::get().ds.RL() << ", not " << rl);
}

void Construction::run(conj_graph_pack &gp, const char*) {
    //    if (cfg::get().etalon_graph_mode) {
    //      typedef io::VectorReader<io::SingleRead> GenomeStream;
    //      GenomeStream genome_stream(io::SingleRead("genome", gp.genome.str()));
    //      std::vector <ReadStream*> streams(1, &genome_stream);
    //      construct_graph(streams, gp);
    //    } else

    // Has to be separate stream for not counting it in coverage
    ReadStream* additional_contigs_stream = 0;
    if (cfg::get().use_additional_contigs) {
        INFO("Contigs from previous K will be used");
        additional_contigs_stream =
                new io::EasyReader(cfg::get().additional_contigs, true);
    }

    std::vector<size_t> libs_for_construction;
    for (size_t i = 0; i < cfg::get().ds.reads.lib_count(); ++i)
        if (cfg::get().ds.reads[i].type() == io::LibraryType::PairedEnd ||
            cfg::get().ds.reads[i].type() == io::LibraryType::SingleReads)
            libs_for_construction.push_back(i);

    if (cfg::get().use_multithreading) {
        auto streams = single_binary_readers_for_libs(libs_for_construction, true, true);
        construct_graph<io::SingleReadSeq>(*streams, gp, additional_contigs_stream);
    } else {
        auto single_stream = single_easy_reader_for_libs(libs_for_construction, true, true);
        io::ReadStreamVector<ReadStream> streams(single_stream.get());
        single_stream.release();
        construct_graph<io::SingleRead>(streams, gp, additional_contigs_stream);
    }
}

} //namespace debruijn_graph