#pragma once

#include "expectation_maximization.hpp"
#include "util/util.hpp"


#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>

constexpr uint8_t NO_GENOTYPE = std::numeric_limits<uint8_t>::max();

/**
 * Contains an entry of the Varsim "map" file, which maps the genome fasta generated by Varsim (with
 * both maternal and paternal entries) to the original genome fasta.
 */
struct ChrMap {
    uint8_t chromosome_id; // 0 to 23, 22=X 23=Y
    uint32_t start_pos; // position where the transformation is applied on the new genome
    uint32_t len; // length of the transformation
    // type of transformation. We only care about I=insert (positions inserted in the new
    // genome, not found in the reference), D=delete (positions in reference missing in the
    // new genome)
    char tr;
};

/**
 * Reads the next chromosome from a FASTA file that contains pairs of (maternal,paternal) contigs
 */
void get_next_chromosome(std::ifstream &fasta_file,
                         const std::unordered_map<std::string, std::vector<ChrMap>> &map,
                         bool is_diploid,
                         std::vector<uint8_t> *chr_data,
                         std::vector<uint8_t> *tmp1,
                         std::vector<uint8_t> *tmp2);

/**
 * Calls the most likely variant for each position for each of the clusters given by #clusters.
 * @param pos_data pooled reads for each chromosome at each position; chromosomes are indexed from
 * 0 to 23, with 22 representing chr X and 23 chr Y.
 * @param clusters the cluster to which each cell belongs to
 * @param reference_genome the genome to call the variants against
 * @param map_file maps positions in the diploid #reference_genome to a haploid ancestor
 * @param hetero_prior the probability that a locus is heterozygous
 * @param theta sequencing error rate
 * @param out_dir location where to write the VCF files that describe the variants
 */
void variant_calling(const std::vector<std::vector<PosData>> &pos_data,
                     const std::vector<uint16_t> &clusters,
                     const std::string &reference_genome,
                     const std::string &map_file,
                     double hetero_prior,
                     double theta,
                     const std::filesystem::path &out_dir);

/**
 * Reads a Varsim map file, which maps a generated diploid genome to the original haploid reference.
 * The structure of the Varsim map file is:
 * <size_of_block> <host_chr> <host_loc> <ref_chr> <ref_loc> <direction_of_block> <feature_name>
 * <variant_id>
 * @param map_file
 * @return a map of chromosome name to list of Insertion/Deletion events
 */
std::unordered_map<std::string, std::vector<ChrMap>> read_map(const std::string &map_file);

/** Applies the given map to #chr_data and places the result in #new_chr_data */
void apply_map(const std::vector<ChrMap> &map,
               const std::vector<uint8_t> &chr_data,
               std::vector<uint8_t> *new_chr_data);

/**
 * Determines if the reference genome used to call variants is diploid (as generated by Varsim) or
 * haploid (as most reference genomes are). It uses a very simple rule: if the first chromosome
 * contains the word "maternal" the genome is deemed diploid (because Varsim calls the first
 * chromosome `>1_maternal`)
 * @VisibleForTesting
 */
bool check_is_diploid(std::ifstream &f);

/**
 * Find the most likely genotype, if we observe bases counts
 * @parm nBases (array of length 4: number of As, Cs, etc.)
 * @param n_bases_total total number for each base at the locus (for all clusters)
 * @param nbases_total_idx idx to sort n_bases_total in increasing order
 * @param likely_homozygous_total true if we are likely dealing with a homozygous locus (almost all
 * bases are identical)
 * @param hetero_prior the prior on heterozygous genotype
 * @param theta sequencing error rate
 * @param[out] coverage if not null, the function will write the total coverage to this value
 * @VisibleForTesting
 */
uint8_t most_likely_genotype(const std::array<uint16_t, 4> &nBases,
                             const std::array<uint16_t, 4> &n_bases_total,
                             const std::array<uint32_t, 4> &nbases_total_idx,
                             bool likely_homozygous_total,
                             double hetero_prior,
                             double theta,
                             uint16_t *coverage = nullptr);

/**
 * Checks if a locus is likely homozygous and returns the homozygous genotype if yes.
 * A locus is declared homozygous if the number of the non-dominant bases is no more than one
 * standard deviation away from the expected number (given theta, the sequencing error rate).
 * @VisibleForTesting
 */
uint8_t likely_homozygous(const std::array<uint16_t, 4> &nBases, double theta);
