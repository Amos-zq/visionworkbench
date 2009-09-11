#ifndef __VW_PLATEFILE_INDEX_H__
#define __VW_PLATEFILE_INDEX_H__

#include <vector>
#include <boost/smart_ptr.hpp>

#include <vw/Core/FundamentalTypes.h>
#include <vw/Core/Exception.h>
#include <vw/Core/Thread.h>
#include <vw/Plate/Tree.h>
#include <vw/Plate/Blob.h>

namespace vw {
namespace platefile {
  
  // -------------------------------------------------------------------
  //                          INDEX_RECORD
  // 
  // The IndexRecord stores all of the metadata needed to recover an
  // image from the blob.  It will be marked as valid if it contains
  // good data, or as invalid if it does not.  Records may be marked
  // as invalid if they represent low-res tiles that need to be
  // re-renedered from higher-res tiles that have been updated.
  // -------------------------------------------------------------------

  #define VW_PLATE_INDEXRECORD_FILETYPE_SIZE 5

  class IndexRecord {

    int32 m_blob_id;
    int64 m_blob_offset;
    int32 m_block_size;
    uint8 m_block_filetype[VW_PLATE_INDEXRECORD_FILETYPE_SIZE];
    uint8 m_valid;

  public:

    IndexRecord() : m_valid(false) {}

    IndexRecord(int32 blob_id, int64 blob_offset, 
                int32 block_size, std::string block_filetype) :
      m_blob_id(blob_id), m_blob_offset(blob_offset), 
      m_block_size(block_size), m_valid(true) {

      if (block_filetype.size() > 4) 
        vw_throw(ArgumentErr() << "IndexRecord: filetype argument must be 4 characters or fewer.");
      strncpy((char*)m_block_filetype, block_filetype.c_str(), 5);

    }

    IndexRecord(std::istream &istr) {
      this->deserialize(istr);
    }


    int32 blob_id() const { return m_blob_id; }
    void set_blob_id(int32 blob_id) { m_blob_id = blob_id; }
    
    int64 blob_offset() const { return m_blob_offset; }
    void set_blob_offset(int64 blob_offset) { m_blob_offset = blob_offset; }

    int32 block_size() const { return m_block_size; }
    void set_block_size(int32 block_size) { m_block_size = block_size; }

    std::string block_filetype() const { return (char*)m_block_filetype; }
    void set_block_filetype(std::string block_filetype) {
      if (block_filetype.size() > 4) 
        vw_throw(ArgumentErr() << "IndexRecord: filetype argument must be 4 characters or fewer.");
      strncpy((char*)m_block_filetype, block_filetype.c_str(), 5);
    }

    bool valid() const { return m_valid; }
    void set_valid(bool valid) { m_valid = valid; }

    /// Serialize the index record as a series of bytes.
    void serialize(std::ostream &ostr) const {
      ostr.write( (char*)&m_blob_id, sizeof(m_blob_id));
      ostr.write( (char*)&m_blob_offset, sizeof(m_blob_offset));
      ostr.write( (char*)&m_block_size, sizeof(m_block_size));
      uint8 filetype_size = VW_PLATE_INDEXRECORD_FILETYPE_SIZE;
      ostr.write( (char*)&filetype_size , sizeof(filetype_size));
      for (int i = 0; i < filetype_size; ++i) 
        ostr.write( (char*)&(m_block_filetype[i]), sizeof(*m_block_filetype));
      ostr.write( (char*)&m_valid, sizeof(m_valid));
    }

    /// Deserialize the index record from a stream of bytes
    void deserialize(std::istream &istr) {
      istr.read( (char*)&m_blob_id, sizeof(m_blob_id) );
      istr.read( (char*)&m_blob_offset, sizeof(m_blob_offset) );
      istr.read( (char*)&m_block_size, sizeof(m_block_size) );
      uint8 filetype_size;
      istr.read( (char*)&filetype_size, sizeof(filetype_size) );
      for (int i = 0; i < filetype_size; ++i) 
        istr.read( (char*)&(m_block_filetype[i]), sizeof(*m_block_filetype));
      istr.read( (char*)&m_valid, sizeof(m_valid));
    }

  };


  // -------------------------------------------------------------------
  //                            PLATE_FILE
  // -------------------------------------------------------------------

  class Index { 

    boost::shared_ptr<BlobManager> m_blob_manager;
    boost::shared_ptr<TreeNode<IndexRecord> > m_root;
    Mutex m_mutex;

  public:

    /// Create a new index.  Uses default blob manager.
    Index() :
      m_blob_manager( new BlobManager() ), 
      m_root(boost::shared_ptr<TreeNode<IndexRecord> >( new TreeNode<IndexRecord>() )) {}

    /// Create a new index.  User supplies a pre-configure blob manager.
    Index( boost::shared_ptr<BlobManager> blob_manager) :
      m_blob_manager(blob_manager), 
      m_root(boost::shared_ptr<TreeNode<IndexRecord> >( new TreeNode<IndexRecord>() )) {}

    /// Open an existing index. 
    Index(std::string index_filename) {
      std::cout << "OPENING A PLATE INDEX IS NOT YET IMPLEMENTED!!!\n";
      exit(0);
    }

    void save(std::string filename) {
      std::cout << "SAVING A PLATE INDEX IS NOT YET IMPLEMENTED!!!\n";      
      exit(0);
    }

    /// Attempt to access a tile in the index.  Throws an
    /// TileNotFoundErr if the tile cannot be found.
    IndexRecord read_request(int col, int row, int depth) {
      Mutex::Lock lock(m_mutex);
      return m_root->search(col, row, depth);
    }
  
    // Writing, pt. 1: Locks a blob and returns the blob id that can
    // be used to write a block.
    int write_request(int size) {  
      return m_blob_manager->request_lock(size); 
    }

    // Writing, pt. 2: Supply information to update the index and
    // unlock the blob id.
    void write_complete(int col, int row, int depth, IndexRecord record) {
      m_blob_manager->release_lock(record.blob_id()); 
      {
        Mutex::Lock lock(m_mutex);
        m_root->insert(record, col, row, depth);
      }
    }
  };

}} // namespace vw::plate

#endif // __VW_PLATEFILE_INDEX_H__