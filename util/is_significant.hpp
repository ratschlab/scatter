#pragma once

#include "sequenced_data.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

/** Marks a cell id that doesn't belong to the current subcluster */
constexpr uint16_t NO_POS = std::numeric_limits<uint16_t>::max() >> 2;

class Filter {
  public:
    Filter();

    /**
     * Decides if a given position is worth keeping, i.e. it will be useful in distinguishing cell
     * genotypes.
     * @param base_count counts of A,C,G, and T in the pooled data at a fixed position
     * @param theta sequencing error rate (e.g. ~0.01 on Illumina machines)
     * @return true if the position is kept
     */
    bool is_significant(std::array<uint16_t, 4> &base_count, double theta);

    bool is_significant(const PosData &pos_data, double theta, uint16_t *coverage);

    /**
     * Filters the positions in #pos_data by keeping only the positions that are relevant
     * (homozygous and not consistent with the null hypothesis of ‘all the cells have the same
     * genotype at this position’)
     * @param pos_data pileup data containing all positions where not all nucleotides are identical
     * across all cells
     * @param id_to_group of size n_cells maps cell ids to cell groups. Data from cells in the same
     * group is treated as if it came from one cell. Used to artificially increase coverage when
     * testing
     * @param id_to_pos of size n_groups maps a cell id to its position in the similarity matrix as
     * we subdivide into smaller and smaller clusters. At the beginning, this is the identity
     * permutation. If a cell with id 'cell_id' is not in the current cluster, then
     * id_to_pos[cell_id]==NO_POS. The position of a cell in the similarity matrix is given by
     * id_to_pos[id_to_group[cell_id]].
     * @param marker marks the current sub-cluster; for example AB means we are in the second
     * sub-cluster (B) of the first cluster (A)
     * @param seq_error_rate rate of the sequencer, e.g. 1e-3 if using Illumina reads with base
     * quality >=30
     * @return the positions in pos_data that are relevant for the current subcluster and the
     * average coverage of the subcluster
     */
    std::pair<std::vector<std::vector<PosData>>, double>
    filter(const std::vector<std::vector<PosData>> &pos_data,
           const std::vector<uint16_t> &id_to_group,
           const std::vector<uint32_t> &id_to_pos,
           const std::string &marker,
           double seq_error_rate,
           uint32_t num_threads);

    /**
     * Computes the log factorial using a table for small values or Stirling's formula for larger
     * values.
     * @VisibleForTesting
     */
    double log_fact(uint32_t n);
};
