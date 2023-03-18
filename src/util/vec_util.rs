use std::ops::Deref;
use std::ops::DerefMut;
use std::ops::Index;
use std::ops::IndexMut;

use std::marker::PhantomData;

#[derive(Hash)]
pub struct VecId<T>(u32, PhantomData<T>);

impl<T> PartialEq for VecId<T> {
    fn eq(&self, other: &Self) -> bool {
        self.0 == other.0
    }
}

impl<T> From<u32> for VecId<T> {
    fn from(id: u32) -> Self {
        VecId(id, PhantomData)
    }
}

impl<T> From<usize> for VecId<T> {
    fn from(id: usize) -> Self {
        assert!(id <= u32::MAX as usize);
        VecId(id as u32, PhantomData)
    }
}

impl<T> Eq for VecId<T> {}

impl<T> Clone for VecId<T> {
    fn clone(&self) -> Self {
        VecId(self.0, PhantomData)
    }
}

impl<T> Copy for VecId<T> {

}

impl<T> std::fmt::Debug for VecId<T> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_fmt(format_args!("Index<{}>: {}", std::any::type_name::<T>(), self.0))
    }
}

impl<T> ToString for VecId<T> {
    fn to_string(&self) -> String {
        self.0.to_string()
    }
}

pub trait IndexedVector {
    type Index;
}

pub struct IVec<T>(Vec<T>);

impl<T> IndexedVector for IVec<T> {
    type Index = VecId<T>;
}

impl<T> IVec<T> {
    pub fn valid_index(&self, idx: VecId<T>) -> bool {
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

impl<T> Index<VecId<T>> for IVec<T> {
    type Output = T;
    
    fn index(&self, index: VecId<T>) -> &Self::Output {
        &self.0[index.0 as usize]
    }
}

impl<T> IndexMut<VecId<T>> for IVec<T> {
    fn index_mut(&mut self, index: VecId<T>) -> &mut Self::Output {
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