using System;
using System.Diagnostics;
using System.Windows.Forms;
using System.Collections;
//using System.Collections.Specialized;

namespace StCounter
{
	/// <summary>
	/// Summary description for PerformanceCtr.
	/// </summary>
	public class PerformanceCtr
	{
		/// <summary>
		/// The class constructor. </summary>
		public PerformanceCtr()
		{
//			InitCounters();
//			AttachCounters();
		}
		private static PerformanceCounter PC_sectors;
		private static PerformanceCounter PC_bytes_per_sec;
		public struct 	ReadCounterData
		{
			public float rate;
			public long bytes;
			public long sectors;
		};

		/// <summary>
		/// One time construction of the PerformanceCounterCategory. </summary>
		public static bool InitCounters()
		{        
			if ( !PerformanceCounterCategory.Exists("StLogicalDisk") ) 
			{

				CounterCreationDataCollection CCDC = new CounterCreationDataCollection();
            
				// Add the bytes counter.
				CounterCreationData sectors = new CounterCreationData();
				sectors.CounterName = "Disk Read Sectors";
				sectors.CounterType = PerformanceCounterType.NumberOfItems64;
				sectors.CounterHelp = "Disk Read Sectors is the number of sectors read from the disk.";

				// Add the Disk Read Bytes/sec counter.
				CounterCreationData diskReadRate = new CounterCreationData();
				diskReadRate.CounterName = "Disk Read Bytes/sec";
				diskReadRate.CounterType = PerformanceCounterType.RateOfCountsPerSecond64;
				diskReadRate.CounterHelp = "Disk Read Bytes/sec is the rate at which bytes are transferred from the disk during read operations.";
				
				CCDC.Add(diskReadRate);
				CCDC.Add(sectors);

				// Create the category.
				PerformanceCounterCategory category;
				System.Collections.ArrayList counters = new System.Collections.ArrayList();
				category = PerformanceCounterCategory.Create("StLogicalDisk", 
						"The StLogicalDisk performance object consists of counters that monitor logical partitions of disk drives.  Performance Monitor identifies logical disks by their a drive letter, such as E:.", 
						CCDC);
				PerformanceCounter.CloseSharedResources();
				Trace.WriteLine("Created Category - StLogicalDisk");
                
				return(true);
			}
			else
			{
				Trace.WriteLine("Category exists - StLogicalDisk");
				return(false);
			}

		}

		/// <summary>
		/// Attaches the counters to the member variables. </summary>
		public static void AttachCounters(/*string _drive*/)
		{
/*			if ( PerformanceCounterCategory.Exists("StLogicalDisk") ) 
			{
				Trace.WriteLine("Category exists - StLogicalDisk");
				if ( PerformanceCounterCategory.CounterExists("StLogicalDisk", "Disk Read Sectors" ) )
					Trace.WriteLine("	Counter 'Disk Read Sectors' exist");
				if ( PerformanceCounterCategory.CounterExists("StLogicalDisk", "Disk Read Sectors" ) )
					Trace.WriteLine("	Counter 'Disk Read Bytes/sec' exist");
			}
*/			// Create the counters.
			PC_sectors = new PerformanceCounter("StLogicalDisk", 
				"Disk Read Sectors", false);

			PC_bytes_per_sec = new PerformanceCounter("StLogicalDisk", 
				"Disk Read Bytes/sec", false);
        
			PC_sectors.RawValue = 0;
			PC_bytes_per_sec.RawValue = 0;

			Trace.WriteLine("Attached counters.");
		}
		/// <summary>
		/// Increments the counters using #sectors as input. </summary>
		/// <param name="_sectors"> number of sectors read fo this operation"</param>
		public static void BumpCounters(long _sectors)
		{
			// bump.
			PC_sectors.IncrementBy(_sectors);
			PC_bytes_per_sec.IncrementBy(_sectors*528);
//			cs_old = PC_bytes_per_sec.NextSample();
		}

		/// <summary>
		/// Increments the counters using #sectors as input. </summary>
		/// <param name="_rate"> returned bytes/sec </param>
		/// <param name="_bytes"> returned total bytes read </param>
		/// <param name="_sectors"> returned total number of sectors read</param>
		public ReadCounterData ReadCounters(/*float* _rate, long* _bytes, long* _sectors*/)
		{
			ReadCounterData data = new ReadCounterData();
			// read.
			data.sectors = (long)PC_sectors.NextValue();
//			*_sectors = (long)sectors;
			data.bytes = PC_bytes_per_sec.RawValue;
//			*_bytes = bytes;
//			CounterSample s0;
//			s0 = PC_bytes_per_sec.NextSample();
			data.rate = PC_bytes_per_sec.NextValue();
//			float rate = CounterSampleCalculator.ComputeCounterValue(cs_old, s0);
//			*_rate = rate;
//			cs_old = s0;
			return data;
		}
	}
}
