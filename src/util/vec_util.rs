use std::ops::Deref;
use std::ops::DerefMut;
use std::ops::Index;
use std::ops::IndexMut;

#[derive(Debug, Copy, Clone, Hash)]
pub struct VecId(u32);

impl PartialEq for VecId {
    fn eq(&self, other: &Self) -> bool {
        self.0 == other.0
    }
}

impl From<u32> for VecId {
    fn from(id: u32) -> Self {
        VecId(id)
    }
}

impl From<usize> for VecId {
    fn from(id: usize) -> Self {
        assert!(id <= u32::MAX as usize);
        VecId(id as u32)
    }
}

impl Eq for VecId {}

impl ToString for VecId {
    fn to_string(&self) -> String {
        self.0.to_string()
    }
}

pub struct IVec<T>(Vec<T>);

impl<T> IVec<T> {
    pub fn valid_index(&self, idx: VecId) -> bool {
        (idx.0 as usize) < self.len()
    }
}

impl<T> Deref for IVec<T> {
    type Target = Vec<T>;

    fn deref(&self) -> &Self::Target {
        &self.0
    }
}

impl<T> DerefMut for IVec<T> {
    fn deref_mut(&mut self) -> &mut Self::Target {
        &mut self.0
    }
}

impl<T> Index<VecId> for IVec<T> {
    type Output = T;
    
    fn index(&self, index: VecId) -> &Self::Output {
        &self.0[index.0 as usize]
    }
}

impl<T> IndexMut<VecId> for IVec<T> {
    fn index_mut(&mut self, index: VecId) -> &mut Self::Output {
        &mut self.0[index.0 as usize]
    }
}

impl<T> From<Vec<T>> for IVec<T> {
    fn from(value: Vec<T>) -> Self {
        IVec(value)
    }
}

#[macro_export]
macro_rules! ivec {
    ($($x:tt)*) => {
        IVec::from(vec![$($x)*])
    }
}
pub use ivec;